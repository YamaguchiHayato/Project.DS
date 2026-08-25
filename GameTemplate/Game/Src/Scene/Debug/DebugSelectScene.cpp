#include "stdafx.h"
#include "DebugSelectScene.h"
#include "Src/Scene/GameFlow.h"

namespace
{
	const Vector3 vTitleFontPos_ = { -300.0f, 300.0f, 0.0f };			//! 見出しの表示位置。
	const Vector3 vEnemySoloFontPos_ = { -300.0f, 150.0f, 0.0f };	//! 敵AIテスト項目の表示位置。
	const Vector3 vShootingRangeFontPos_ = { -300.0f, 50.0f, 0.0f };	//! 射撃場テスト項目の表示位置。
	const Vector3 vBackFontPos_ = { -300.0f, -80.0f, 0.0f };			//! 戻る項目の表示位置。
	const float fTitleFontScale_ = 1.5f;							//! 見出しフォントの大きさ。
	const float fMenuFontScale_ = 1.2f;								//! 項目フォントの大きさ。
}

namespace nsApp
{
	namespace nsScene
	{
		bool DebugSelectScene::Start()
		{
			/* 見出しを初期化する。*/
			stTitleFont_.SetPosition(vTitleFontPos_);
			stTitleFont_.SetScale(fTitleFontScale_);
			stTitleFont_.SetColor(1.0f, 1.0f, 0.0f, 1.0f);
			stTitleFont_.SetText(L"DEBUG SELECT");

			/* 敵AIテスト項目を初期化する。*/
			stEnemySoloFont_.SetPosition(vEnemySoloFontPos_);
			stEnemySoloFont_.SetScale(fMenuFontScale_);
			stEnemySoloFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stEnemySoloFont_.SetText(L"1   ENEMY AI");

			/* 射撃場テスト項目を初期化する。*/
			stShootingRangeFont_.SetPosition(vShootingRangeFontPos_);
			stShootingRangeFont_.SetScale(fMenuFontScale_);
			stShootingRangeFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stShootingRangeFont_.SetText(L"2   SHOOTING RANGE");

			/* 戻る項目を初期化する。*/
			stBackFont_.SetPosition(vBackFontPos_);
			stBackFont_.SetScale(fMenuFontScale_);
			stBackFont_.SetColor(0.7f, 0.7f, 0.7f, 1.0f);
			stBackFont_.SetText(L"ESC BACK");

			/* 遷移直後に押しっぱなしのキーを新規入力扱いにしない。*/
			bWasPress1_ = (GetAsyncKeyState('1') & 0x8000) != 0;
			bWasPress2_ = (GetAsyncKeyState('2') & 0x8000) != 0;
			bWasPressEsc_ = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

			return true;
		}


		void DebugSelectScene::Update()
		{
			/* 今フレームのキー状態を取る。*/
			const bool bPress1 = (GetAsyncKeyState('1') & 0x8000) != 0;
			const bool bPress2 = (GetAsyncKeyState('2') & 0x8000) != 0;
			const bool bPressEsc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

			/* キー1を押した瞬間だけ敵AIテストへ遷移予約。*/
			if (bPress1 && !bWasPress1_)
				pGameFlow_->ChangeScene(EnSceneID::Debug, EnDebugSceneID::EnemySolo);

			/* キー2を押した瞬間だけ射撃場テストへ遷移予約。*/
			if (bPress2 && !bWasPress2_)
				pGameFlow_->ChangeScene(EnSceneID::Debug, EnDebugSceneID::ShootingRange);

			/* ESCを押した瞬間だけタイトルへ戻る。*/
			if (bPressEsc && !bWasPressEsc_)
				pGameFlow_->ChangeScene(EnSceneID::Title);

			/* 次フレーム判定用に今の状態を残す。*/
			bWasPress1_ = bPress1;
			bWasPress2_ = bPress2;
			bWasPressEsc_ = bPressEsc;
		}


		void DebugSelectScene::Render(RenderContext& rc)
		{
			/* 見出しを描画する。*/
			stTitleFont_.Draw(rc);

			/* 項目を描画する。*/
			stEnemySoloFont_.Draw(rc);
			stShootingRangeFont_.Draw(rc);
			stBackFont_.Draw(rc);
		}
	}
}