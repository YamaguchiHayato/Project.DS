#pragma once
#include "OptionEntry.h"

namespace nsApp
{
	namespace nsUI
	{
		class OptionCanvas;

		/**
		 * @file   OptionTabBar.h
		 * @brief  設定画面の上のタイトルとタブ列。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 新規作成日。
		 */
		class OptionTabBar
		{
		public:
			/**
			 * @brief タイトルとタブを描く。
			 * @param rc 描画コンテキスト。
			 * @param canvas 描画先。
			 * @param current 今のタブ。
			 */
			void Draw(RenderContext& rc, OptionCanvas& canvas, OptionTab current)const;

			/**
			 * @brief 点が乗っているタブを返す。
			 * @param point 点。スプライト空間。
			 * @return タブの番号。乗っていなければ -1。
			 */
			int HitTab(const Vector2& point)const;

			/**
			 * @brief 点が乗っている左右の矢印を返す。
			 * @param point 点。スプライト空間。
			 * @return 左なら -1、右なら 1、乗っていなければ 0。
			 */
			int HitArrow(const Vector2& point)const;
		};
	}
}