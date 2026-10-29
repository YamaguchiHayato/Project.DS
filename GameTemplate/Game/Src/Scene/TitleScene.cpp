#include "stdafx.h"
#include "TitleScene.h"
#include "system/system.h"
#include "Src/Scene/GameFlow.h"
#include "Src/Scene/IScene.h"
#include "Src/UI/Title/TitleUI.h"
#include "Src/UI/Title/TitleStage.h"
#include "Src/UI/KeyConfig/KeyConfigUI.h"
#include "Src/InputObserver.h"
#include "Src/UI/VideoScreen.h"

#pragma comment(lib, "Shell32.lib")

namespace
{
	/* GITHUB の行で開くページ。@todo 公開用のページを分けるなら差し替える。*/
	const wchar_t* sRepositoryUrl_ = L"https://github.com/YamaguchiHayato/Project.DS";

	/**
	 * @brief 既定のブラウザでリポジトリのページを開く。
	 * @details 開けなくてもタイトルはそのまま続ける。ゲームを止めるほどのことではないため。
	 */
	void OpenRepositoryPage()
	{
		ShellExecuteW(nullptr, L"open", sRepositoryUrl_, nullptr, nullptr, SW_SHOWNORMAL);
	}
}

namespace nsApp
{
	namespace nsScene
	{
		TitleScene::~TitleScene()
		{
			/* 生成したタイトル本体を破棄する。*/
			if (pTitleUI_ != nullptr)
			{
				DeleteGO(pTitleUI_);
				pTitleUI_ = nullptr;
			}

			/* 立たせていたモデルを破棄する。カメラとライトはこの中で元へ戻る。*/
			if (pTitleStage_ != nullptr)
			{
				DeleteGO(pTitleStage_);
				pTitleStage_ = nullptr;
			}

			if (pKeyConfig_ != nullptr)
			{
				DeleteGO(pKeyConfig_);
				pKeyConfig_ = nullptr;
			}

			if (pVideoScreen_ != nullptr)
			{
				DeleteGO(pVideoScreen_);
				pVideoScreen_ = nullptr;
			}
		}


		bool TitleScene::Start()
		{
			/* タイトルの前に動画を流す。タイトル本体は動画が終わってから生成する。*/
			pVideoScreen_ = NewGO<nsUI::VideoScreen>(0, "videoScreen");
			return true;
		}


		void TitleScene::Update()
		{
			/* 動画の再生中は、タイトルの入力を受け付けない。*/
			if (UpdateVideo())
				return;

			/* 設定画面を開いている間も、タイトルの入力を受け付けない。*/
			if (UpdateKeyConfig())
				return;

			/* メニューで決まった行き先を実行する。*/
			if (pTitleUI_ != nullptr)
				ApplyTitleCommand(pTitleUI_->PopDecidedCommand());

			/* デバッグシーンへの近道。メニューには出していないので、キーだけで入れるようにしている。*/
			if (nsData::InputObserver::Get().IsTrigger(nsData::EnAction::OpenDebug))
				pGameFlow_->ChangeScene(EnSceneID::Debug);
		}


		bool TitleScene::UpdateVideo()
		{
			/* 動画を流していなければ、何もせずに先へ進ませる。*/
			if (pVideoScreen_ == nullptr)
				return false;

			/* 決定キーか再生終了までは、動画を流し続ける。*/
			if (!nsData::InputObserver::Get().IsTrigger(nsData::EnAction::Confirm) && !pVideoScreen_->IsEnd())
				return true;

			DeleteGO(pVideoScreen_);
			pVideoScreen_ = nullptr;

			CreateTitle();
			return true;
		}


		bool TitleScene::UpdateKeyConfig()
		{
			/* 設定画面を開いていなければ、何もせずに先へ進ませる。*/
			if (pKeyConfig_ == nullptr)
				return false;

			/* 閉じられるまでは開いたままにする。*/
			if (!pKeyConfig_->IsClose())
				return true;

			DeleteGO(pKeyConfig_);
			pKeyConfig_ = nullptr;

			/* 閉じたので、タイトルの入力を元に戻す。*/
			if (pTitleUI_ != nullptr)
				pTitleUI_->SetInputActive(true);

			return true;
		}


		void TitleScene::CreateTitle()
		{
			/* 2D表示(ロゴ・メニュー)を生成する。*/
			if (pTitleUI_ == nullptr)
				pTitleUI_ = NewGO<nsUI::TitleUI>(0, "title");

			/* 3D表示(立たせるモデル)を生成する。*/
			if (pTitleStage_ == nullptr)
				pTitleStage_ = NewGO<nsUI::TitleStage>(0, "titleStage");
		}


		void TitleScene::ApplyTitleCommand(nsUI::EnTitleCommand enCommand)
		{
			switch (enCommand)
			{
			case nsUI::EnTitleCommand::StartGame:
				/* 本編へ切り替える。*/
				pGameFlow_->ChangeScene(EnSceneID::InGame);
				break;

			case nsUI::EnTitleCommand::Option:
				/* 設定はキーコンフィグを開く。開いている間はタイトルの入力を止める。*/
				pKeyConfig_ = NewGO<nsUI::KeyConfigUI>(0, "keyConfig");
				if (pTitleUI_ != nullptr)
					pTitleUI_->SetInputActive(false);
				break;

			case nsUI::EnTitleCommand::OpenGitHub:
				/* リポジトリのページをブラウザで開く。ゲームはタイトルのまま続ける。*/
				OpenRepositoryPage();
				break;

			case nsUI::EnTitleCommand::ExitGame:
				/* ゲームループを抜けてウィンドウを閉じる。*/
				g_gameLoop.m_isLoop = false;
				break;

			default:
				/* まだ何も決まっていないときは何もしない。*/
				break;
			}
		}
	}
}
