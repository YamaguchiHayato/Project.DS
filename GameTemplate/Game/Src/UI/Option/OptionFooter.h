#pragma once
#include "OptionEntry.h"

namespace nsApp
{
	namespace nsUI
	{
		class OptionCanvas;

		/**
		 * @file   OptionFooter.h
		 * @brief  設定画面の下の説明文と操作ガイド。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 新規作成日。
		 */
		class OptionFooter
		{
		public:
			/**
			 * @brief 説明文と操作ガイドを描く。
			 * @param rc 描画コンテキスト。
			 * @param canvas 描画先。
			 * @param entry 選んでいる行。無ければ nullptr。
			 */
			void Draw(RenderContext& rc, OptionCanvas& canvas, const OptionEntry* entry)const;
		};
	}
}