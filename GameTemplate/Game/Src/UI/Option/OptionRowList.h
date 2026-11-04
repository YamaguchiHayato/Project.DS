#pragma once
#include "OptionEntry.h"

namespace nsApp
{
	namespace nsUI
	{
		class OptionCanvas;
		class OptionModel;

		/**
		 * @file   OptionRowList.h
		 * @brief  設定画面の項目の行。項目名、値、ゲージ、選択中の帯と矢印を描く。
		 * @details 画面に入りきらない行は top から MAX_VISIBLE 行ぶんだけ描く。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 新規作成日。
		 */
		class OptionRowList
		{
		public:
			static constexpr int MAX_VISIBLE = 9; //! 一度に見せる行の数。


		public:
			/**
			 * @brief 行を描く。
			 * @param rc 描画コンテキスト。
			 * @param canvas 描画先。
			 * @param model 値の読み出し。
			 * @param rows 今のタブの行。
			 * @param rowCount 行の数。
			 * @param top 一番上に見せる行の番号。
			 * @param cursor 選んでいる行の番号。
			 */
			void Draw(RenderContext& rc, OptionCanvas& canvas, const OptionModel& model, const OptionEntry* const* rows, int rowCount, int top, int cursor)const;

			/**
			 * @brief 点が乗っている行を返す。
			 * @param point 点。スプライト空間。
			 * @param rowCount 行の数。
			 * @param top 一番上に見せている行の番号。
			 * @return 行の番号。乗っていなければ -1。
			 */
			int HitRow(const Vector2& point, int rowCount, int top)const;

			/**
			 * @brief 選んでいる行の左右の矢印に点が乗っているかを返す。
			 * @param point 点。スプライト空間。
			 * @param top 一番上に見せている行の番号。
			 * @param cursor 選んでいる行の番号。
			 * @return 左なら -1、右なら 1、乗っていなければ 0。
			 */
			int HitArrow(const Vector2& point, int top, int cursor)const;
		};
	}
}