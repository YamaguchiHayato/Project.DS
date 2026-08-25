#include "stdafx.h"
#include "DebugShootingRangeScene.h"
#include "Src/Scene/GameFlow.h"

namespace
{
	const Vector3 vTitleFontPos_ = { -300.0f, 200.0f, 0.0f };	//! 見出しの表示位置。
	const Vector3 vBackFontPos_ = { -300.0f, 50.0f, 0.0f };		//! 戻る項目の表示位置。
	const float fTitleFontScale_ = 1.5f;						//! 見出しフォントの大きさ。
	const float fMenuFontScale_ = 1.2f;							//! 項目フォントの大きさ。
}

namespace nsApp
{
	namespace nsScene
	{
		bool DebugShootingRangeScene::Start()
		{
			/* 見出しを初期化する。*/
			stTitleFont_.SetPosition(vTitleFontPos_);
			stTitleFont_.SetScale(fTitleFontScale_);
			stTitleFont_.SetColor(1.0f, 1.0f, 0.0f, 1.0f);
			stTitleFont_.SetText(L"SHOOTING RANGE");

			/* 戻る項目を初期化する。*/
			stBackFont_.SetPosition(vBackFontPos_);
			stBackFont_.SetScale(fMenuFontScale_);
			stBackFont_.SetColor(0.7f, 0.7f, 0.7f, 1.0f);
			stBackFont_.SetText(L"ESC BACK");

			/* 遷移直後に押しっぱなしのキーを新規入力扱いにしない。*/
			bWasPressEsc_ = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

			return true;
		}


		void DebugShootingRangeScene::Update()
		{
			/* 今フレームのESC状態を取る。*/
			const bool bPressEsc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

			/* ESCを押した瞬間だけDebug選択へ戻る。*/
			if (bPressEsc && !bWasPressEsc_)
				pGameFlow_->ChangeScene(EnSceneID::Debug);

			/* 次フレーム判定用に今の状態を残す。*/
			bWasPressEsc_ = bPressEsc;
		}


		void DebugShootingRangeScene::Render(RenderContext& rc)
		{
			/* 見出しを描画する。*/
			stTitleFont_.Draw(rc);

			/* 戻る項目を描画する。*/
			stBackFont_.Draw(rc);
		}
	}
}