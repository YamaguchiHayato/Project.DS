#include "stdafx.h"
#include "TitleUI.h"

namespace
{
	const char* sTitleImagePath_ = "Assets/image/title/Title.DDS";	//! タイトル画像パス。
	const float fTitleWidth_ = 1920.0f;								//! タイトル画像の横幅。
	const float fTitleHeight_ = 1920.0f;							//! タイトル画像の縦幅。
	const Vector3 vTitlePosition_ = { 0.0f, 120.0f, 0.0f };			//! タイトル画像の表示位置。
	const Vector3 vTitleScale_ = { 1.0f, 1.0f, 1.0f };				//! タイトル画像の拡大率。
	const Vector3 vInGameFontPosition_ = { -180.0f, -220.0f, 0.0f };//! InGame項目の位置。
	const Vector3 vDebugFontPosition_ = { -180.0f, -300.0f, 0.0f };	//! Debug項目の位置。
	const float fMenuFontScale_ = 1.2f;								//! 項目フォントの大きさ。
}

namespace nsApp
{
	namespace nsUI
	{
		bool TitleUI::Start()
		{
			/* タイトル画像スプライトを初期化する。*/
			stTitleSprite_.Init(sTitleImagePath_, fTitleWidth_, fTitleHeight_);
			stTitleSprite_.SetPosition(vTitlePosition_);
			stTitleSprite_.SetScale(vTitleScale_);
			stTitleSprite_.Update();

			/* InGame項目を初期化する。*/
			stInGameFont_.SetPosition(vInGameFontPosition_);
			stInGameFont_.SetScale(fMenuFontScale_);
			stInGameFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stInGameFont_.SetText(L"1   INGAME");

			/* Debug項目を初期化する。*/
			stDebugFont_.SetPosition(vDebugFontPosition_);
			stDebugFont_.SetScale(fMenuFontScale_);
			stDebugFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stDebugFont_.SetText(L"2   DEBUG");
			return true;
		}


		void TitleUI::Update()
		{
			/* 今は固定表示のみ。*/
		}


		void TitleUI::Render(RenderContext& rc)
		{
			/* タイトル画像を描画する。*/
			stTitleSprite_.Draw(rc);

			/* 項目を描画する。*/
			stInGameFont_.Draw(rc);
			stDebugFont_.Draw(rc);
		}
	}
}