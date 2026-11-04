#pragma once

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @file   OptionStyle.h
		 * @brief  設定画面の色と当たり判定。部品どうしで同じ値を使うためにまとめる。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 新規作成日。
		 */
		namespace nsOptionStyle
		{
			inline const Vector4 TEXT_LIGHT = { 0.9f, 0.88f, 0.85f, 1.0f }; //! 暗い地の上の文字。
			inline const Vector4 TEXT_DARK = { 0.08f, 0.06f, 0.06f, 1.0f }; //! 明るい地の上の文字。
			inline const Vector4 SELECT_COLOR = { 0.85f, 0.82f, 0.78f, 1.0f }; //! 選択中の帯とタブ。くすんだ白。
			inline const Vector4 IDLE_COLOR = { 0.12f, 0.12f, 0.13f, 0.9f }; //! 選ばれていない地。
			inline const Vector4 ACCENT_COLOR = { 0.6f, 0.06f, 0.06f, 1.0f }; //! ゲージの中身。暗い赤。
			inline const Vector4 GAUGE_BACK_COLOR = { 0.25f, 0.25f, 0.25f, 1.0f }; //! ゲージの地。
			inline const Vector4 LINE_COLOR = { 0.85f, 0.82f, 0.78f, 1.0f }; //! タブの下の線。
			inline const Vector4 DIM_COLOR = { 0.02f, 0.02f, 0.03f, 0.75f }; //! タイトルの上に重ねる暗い膜。コンフィグの枠とは別の Sprite。

			/**
			 * @brief 点が四角の中にあるかを返す。マウスの当たり判定に使う。
			 * @param point 点。スプライト空間。
			 * @param center 四角の中心。
			 * @param size 四角の幅と高さ。
			 * @return 中にあれば true。
			 */
			inline bool IsInside(const Vector2& point, const Vector2& center, const Vector2& size)
			{
				return fabsf(point.x - center.x) <= size.x * 0.5f
					&& fabsf(point.y - center.y) <= size.y * 0.5f;
			}
		}
	}
}