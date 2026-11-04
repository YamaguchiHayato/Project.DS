#include "stdafx.h"
#include "TitleScene.h"
#include "system/system.h"
#include "Src/Scene/GameFlow.h"
#include "Src/Scene/IScene.h"
#include "Src/UI/Title/TitleUI.h"
#include "Src/InputObserver.h"
#include "Src/UI/VideoScreen.h"


namespace
{
	/* GITHUB の行で開くページ。@todo 公開用のページを分けるなら差し替える。*/
	const wchar_t* sRepositoryUrl_ = L"https://github.com/YamaguchiHayato/Project.DS";
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

			if (pVideoScreen_ != nullptr)
			{
				DeleteGO(pVideoScreen_);
				pVideoScreen_ = nullptr;
			}
		}


		bool TitleScene::Start()
		{
			/* 起動したらすぐにタイトルを出す。動画はタイトルでしばらく操作が無いときに流す。*/
			CreateTitle();
			return true;
		}


		void TitleScene::Update()
		{
			/* 動画の再生中は、タイトルの入力を受け付けない。*/
			if (UpdateVideo())
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

			/* 何かキーを押すか再生が終わるまでは、動画を流し続ける。*/
			const bool bIsSkip = (pTitleUI_ != nullptr) && pTitleUI_->IsAnyKeyTrigger();
			if (!bIsSkip && !pVideoScreen_->IsEnd())
				return true;

			StopDemoVideo();
			return true;
		}


		void TitleScene::StartDemoVideo()
		{
			/* すでに流していれば何もしない。*/
			if (pVideoScreen_ != nullptr)
				return;

			/* タイトルとモデルを隠し、入力も止める。動画を飛ばすキーでメニューが動かないようにするため。*/
			if (pTitleUI_ != nullptr)
			{
				pTitleUI_->SetVisible(false);
				pTitleUI_->SetInputActive(false);
			}

			/* 動画は流すたびに作り直す。VideoScreen は生成時に開いて最後まで読むと終わる作りのため。*/
			pVideoScreen_ = NewGO<nsUI::VideoScreen>(0, "videoScreen");
		}


		void TitleScene::StopDemoVideo()
		{
			/* 動画を片付ける。*/
			if (pVideoScreen_ != nullptr)
			{
				DeleteGO(pVideoScreen_);
				pVideoScreen_ = nullptr;
			}

			/* PRESS ANY KEY の画面に戻して、もう一度表示する。モデルは次のフレームで段階に合わせて隠れる。*/
			if (pTitleUI_ != nullptr)
			{
				pTitleUI_->ResetToPressAnyKey();
				pTitleUI_->SetVisible(true);
				pTitleUI_->SetInputActive(true);
			}
		}


		void TitleScene::CreateTitle()
		{
			/* 2D表示(タイトル画像・メニュー)を生成する。*/
			if (pTitleUI_ == nullptr)
				pTitleUI_ = NewGO<nsUI::TitleUI>(0, "title");

		}


		void TitleScene::ApplyTitleCommand(nsUI::EnTitleCommand enCommand)
		{
			switch (enCommand)
			{
				/* メニューを開く。*/
			case nsUI::EnTitleCommand::OpenMenu:
				pGameFlow_->ChangeScene(EnSceneID::Menu);
				break;

				/* GitHub の行で開くページ。*/
			case nsUI::EnTitleCommand::PlayDemo:
				StartDemoVideo();
				break;

			default:
				/* まだ何も決まっていないときは何もしない。*/
				break;
			}
		}
	}
}
