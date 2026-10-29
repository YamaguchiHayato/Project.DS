#include "stdafx.h"
#include "TitleScene.h"
#include "Src/Scene/GameFlow.h"
#include "Src/Scene/IScene.h"
#include "Src/UI/Title/TitleUI.h"
#include "Src/UI/KeyConfig/KeyConfigUI.h"
#include "Src/InputObserver.h"
#include "Src/UI/VideoScreen.h"

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
			/* タイトル本体を生成する。*/
//			pTitleUI_ = NewGO<nsUI::TitleUI>(0, "title");

			pVideoScreen_ = NewGO<nsUI::VideoScreen>(0, "videoScreen");
			return true;
		}


		void TitleScene::Update()
		{
			/* 動画中は、決定キーか再生終了でタイトルへ進む。*/
			if (pVideoScreen_ != nullptr)
			{
				if (nsData::InputObserver::Get().IsTrigger(nsData::EnAction::Confirm) || pVideoScreen_->IsEnd())
				{
					DeleteGO(pVideoScreen_);
					pVideoScreen_ = nullptr;

					if (pTitleUI_ == nullptr)
						pTitleUI_ = NewGO<nsUI::TitleUI>(0, "title");
				}

				return;
			}

			/* すでにキーコンフィグUIが生成されている場合は、キー入力を受け付けない。*/
			if (pKeyConfig_ != nullptr)
			{
				/* キーコンフィグUIが閉じられたら破棄する。*/
				if (pKeyConfig_->IsClose())
				{
					DeleteGO(pKeyConfig_);
					pKeyConfig_ = nullptr;
				}
				return;
			}

			/* 1キーが押された瞬間なら InGame へ切り替える。*/
			if (nsData::InputObserver::Get().IsTrigger(nsData::EnAction::StartGame))
				pGameFlow_->ChangeScene(EnSceneID::InGame);

			/* 2キーが押された瞬間なら Debug へ切り替える。*/
			if (nsData::InputObserver::Get().IsTrigger(nsData::EnAction::OpenDebug))
				pGameFlow_->ChangeScene(EnSceneID::Debug);

			/* 3キーが押された瞬間なら KeyConfigUI を生成する。*/
			if (pKeyConfig_ == nullptr && nsData::InputObserver::Get().IsTrigger(nsData::EnAction::OpenKeyConfig))
				pKeyConfig_ = NewGO<nsUI::KeyConfigUI>(0, "keyConfig");
		}
	}
}