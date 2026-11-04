#include "stdafx.h"
#include "OptionTabBar.h"
#include "OptionCanvas.h"
#include "OptionStyle.h"

namespace
{
	const wchar_t* TITLE_TEXT = L"オプション"; //! 左上の見出し。
	const Vector2 TITLE_POSITION = { -900.0f, 480.0f }; //! 見出しの位置。
	const float TITLE_SCALE = 0.8f; //! 見出しの大きさ。

	const float TAB_Y = 420.0f; //! タブの縦の位置。
	const float TAB_STEP = 230.0f; //! タブどうしの間隔。
	const Vector2 TAB_SIZE = { 220.0f, 40.0f }; //! タブの大きさ。
	const float TAB_TEXT_SCALE = 0.5f; //! タブの文字の大きさ。

	const float ARROW_OFFSET_X = 50.0f; //! 端のタブの外側から矢印までの距離。
	const float KEY_OFFSET_X = 45.0f; //! 矢印からキー表示までの距離。
	const float ARROW_SCALE = 0.6f; //! 矢印の大きさ。
	const float KEY_SCALE = 0.4f; //! キー表示の大きさ。
	const Vector2 ARROW_HIT_SIZE = { 60.0f, 50.0f }; //! 矢印をクリックできる範囲。

	const float LINE_Y = 392.0f; //! タブの下の線の縦の位置。
	const Vector2 LINE_SIZE = { 1760.0f, 4.0f }; //! タブの下の線の大きさ。

	const int TAB_COUNT = static_cast<int>(nsApp::nsUI::OptionTab::Count); //! タブの数。

	/**
	 * @brief タブの中心の横位置を返す。タブ列は画面の中央にそろえる。
	 * @param index タブの番号。
	 * @return 横位置。
	 */
	float GetTabX(int index)
	{
		return (static_cast<float>(index) - static_cast<float>(TAB_COUNT - 1) * 0.5f) * TAB_STEP;
	}

	/**
	 * @brief 左の矢印の横位置を返す。
	 * @return 横位置。
	 */
	float GetLeftArrowX()
	{
		return GetTabX(0) - TAB_SIZE.x * 0.5f - ARROW_OFFSET_X;
	}

	/**
	 * @brief 右の矢印の横位置を返す。
	 * @return 横位置。
	 */
	float GetRightArrowX()
	{
		return GetTabX(TAB_COUNT - 1) + TAB_SIZE.x * 0.5f + ARROW_OFFSET_X;
	}
}

namespace nsApp
{
	namespace nsUI
	{
		void OptionTabBar::Draw(RenderContext& rc, OptionCanvas& canvas, OptionTab current)const
		{
			/* 見出し。 */
			canvas.DrawString(rc, TITLE_TEXT, TITLE_POSITION, TITLE_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Left);

			/* タブ。選んでいるタブだけ明るい地に暗い文字。 */
			const int selected = static_cast<int>(current);
			for (int index = 0; index < TAB_COUNT; index++)
			{
				const Vector2 center = { GetTabX(index), TAB_Y };
				const bool isSelected = index == selected;
				canvas.DrawBox(center, TAB_SIZE, isSelected ? nsOptionStyle::SELECT_COLOR : nsOptionStyle::IDLE_COLOR);
				canvas.DrawString(rc, OPTION_TAB_LABELS[index], center, TAB_TEXT_SCALE, isSelected ? nsOptionStyle::TEXT_DARK : nsOptionStyle::TEXT_LIGHT, TextAlign::Center);
			}

			/* 左右の矢印と、対応するキー。 */
			const float leftX = GetLeftArrowX();
			const float rightX = GetRightArrowX();
			canvas.DrawString(rc, L"<", { leftX, TAB_Y }, ARROW_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Center);
			canvas.DrawString(rc, L">", { rightX, TAB_Y }, ARROW_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Center);
			canvas.DrawString(rc, L"Q", { leftX - KEY_OFFSET_X, TAB_Y }, KEY_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Center);
			canvas.DrawString(rc, L"E", { rightX + KEY_OFFSET_X, TAB_Y }, KEY_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Center);

			/* タブの下の線。 */
			canvas.DrawBox({ 0.0f, LINE_Y }, LINE_SIZE, nsOptionStyle::LINE_COLOR);
		}


		int OptionTabBar::HitTab(const Vector2& point)const
		{
			for (int index = 0; index < TAB_COUNT; index++)
			{
				if (nsOptionStyle::IsInside(point, { GetTabX(index), TAB_Y }, TAB_SIZE))
					return index;
			}
			return -1;
		}


		int OptionTabBar::HitArrow(const Vector2& point)const
		{
			if (nsOptionStyle::IsInside(point, { GetLeftArrowX(), TAB_Y }, ARROW_HIT_SIZE))
				return -1;

			if (nsOptionStyle::IsInside(point, { GetRightArrowX(), TAB_Y }, ARROW_HIT_SIZE))
				return 1;
			return 0;
		}
	}
}