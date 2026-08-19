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
			/* キー1で本編へ遷移予約。*/
			if (GetAsyncKeyState('1') & 0x8000)
				pGameFlow_->ChangeScene(EnSceneID::InGame);

			/* キー2でデバッグ(敵単体)へ遷移予約。*/
			if (GetAsyncKeyState('2') & 0x8000)
				pGameFlow_->ChangeScene(EnSceneID::Debug, EnDebugSceneID::EnemySolo);
		}
	}
}