#pragma once
#include "OptionEntry.h"
#include "OptionValue.h"

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @file   OptionModel.h
		 * @brief  設定画面の行と GraphicsSetting の橋渡し。表示はしない。
		 * @details 状態を持たず、行の定義を受け取ってその都度 GraphicsSetting を読み書きする。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 新規作成日。
		 */
		class OptionModel
		{
		public:
			/**
			 * @brief 行の値を読む。
			 * @param entry 行。
			 * @return Gauge なら number、Choice なら index に入る。
			 */
			OptionValue Read(const OptionEntry& entry)const;

			/**
			 * @brief 行の値を1刻み動かす。範囲は GraphicsSetting 側で収める。
			 * @param entry 行。
			 * @param direction 右なら 1、左なら -1。
			 */
			void Step(const OptionEntry& entry, int direction)const;

			/**
			 * @brief 行の値を初期値に戻す。
			 * @param entry 行。
			 */
			void Reset(const OptionEntry& entry)const;

			/**
			 * @brief 範囲の中でどこにいるかを返す。ゲージの長さに使う。
			 * @param entry 行。
			 * @return 0.0～1.0。
			 */
			float GetRatio(const OptionEntry& entry)const;

			/**
			 * @brief 行の値を表示用の文字にする。
			 * @param entry 行。
			 * @param text 書き込み先。
			 * @param count 書き込み先の文字数。
			 */
			void Format(const OptionEntry& entry, wchar_t* text, size_t count)const;
		};
	}
}