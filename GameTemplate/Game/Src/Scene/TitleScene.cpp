#include "stdafx.h"
#include "TitleScene.h"
#include "Src/Scene/GameFlow.h"
#include "Src/Scene/IScene.h"
#include "Src/UI/Title/TitleUI.h"

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
		}


		bool TitleScene::Start()
		{
			/* タイトル本体を生成する。*/
			pTitleUI_ = NewGO<nsUI::TitleUI>(0, "title");
			return true;
		}


		void TitleScene::Update()
		{
			/* 今フレームのキー状態を取る。*/
			const bool bPress1 = (GetAsyncKeyState('1') & 0x8000) != 0;
			const bool bPress2 = (GetAsyncKeyState('2') & 0x8000) != 0;

			/* キー1を押した瞬間だけ本編へ遷移予約。*/
			if (bPress1 && !bWasPress1_)
				pGameFlow_->ChangeScene(EnSceneID::InGame);

			/* キー2を押した瞬間だけDebug選択へ遷移予約。*/
			if (bPress2 && !bWasPress2_)
				pGameFlow_->ChangeScene(EnSceneID::Debug);

			/* 次フレーム判定用に今の状態を残す。*/
			bWasPress1_ = bPress1;
			bWasPress2_ = bPress2;
		}
	}
}