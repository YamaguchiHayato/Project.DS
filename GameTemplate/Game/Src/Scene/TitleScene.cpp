#include "stdafx.h"
#include "TitleScene.h"
#include "Src/Scene/GameFlow.h"
#include "Src/Scene/IScene.h"
#include "Src/UI/Title/TitleUI.h"
#include "Src/UI/KeyConfig/KeyConfigUI.h"
#include "Src/InputObserver.h"

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
		}


		bool TitleScene::Start()
		{
			/* タイトル本体を生成する。*/
			pTitleUI_ = NewGO<nsUI::TitleUI>(0, "title");
			return true;
		}


		void TitleScene::Update()
		{
			/* すでにキーコンフィグUIが生成されている場合は、キー入力を受け付けない。*/
			if (pKeyConfig_ != nullptr)
				return;

			/* 今フレームのキー状態を取る。*/
			const bool bPress1 = (GetAsyncKeyState('1') & 0x8000) != 0;
			const bool bPress2 = (GetAsyncKeyState('2') & 0x8000) != 0;

			/* 1キーが押された瞬間なら InGame へ切り替える。*/
			if(nsData::InputObserver::Get().IsTrigger(nsData::EnAction::OpenDebug))
				pGameFlow_->ChangeScene(EnSceneID::Debug);

			/* 2キーが押された瞬間なら Debug へ切り替える。*/
			if (nsData::InputObserver::Get().IsTrigger(nsData::EnAction::StartGame))
				pGameFlow_->ChangeScene(EnSceneID::InGame);

			/* 3キーが押された瞬間なら KeyConfigUI を生成する。*/
			if (pKeyConfig_ == nullptr && nsData::InputObserver::Get().IsTrigger(nsData::EnAction::OpenKeyConfig))
				pKeyConfig_ = NewGO<nsUI::KeyConfigUI>(0, "keyConfig");


			/* 次フレーム判定用に今の状態を残す。*/
			bWasPress1_ = bPress1;
			bWasPress2_ = bPress2;
		}
	}
}