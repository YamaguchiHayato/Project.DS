#include "stdafx.h"
#include "TitleUI.h"

namespace
{
	const char* sTitleImagePath_ = "Assets/image/title/Title.DDS";	//! タイトル画像パス。
	const float fTitleWidth_ = 1920.0f;								//! タイトル画像の横幅。
	const float fTitleHeight_ = 1920.0f;								//! タイトル画像の縦幅。
	const Vector3 vTitlePosition_ = { 0.0f, 120.0f, 0.0f };			//! タイトル画像の表示位置。
	const Vector3 vTitleScale_ = { 1.0f, 1.0f, 1.0f };				//! タイトル画像の拡大率。
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
		}
	}
}