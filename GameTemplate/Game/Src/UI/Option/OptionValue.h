#pragma once

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @file   OptionValue.h
		 * @brief  1行ぶんの値。ゲージは数値、選択肢は番号として読む。
		 * @details 1行はどちらか一方しか使わないので、union で同じ4バイトを共有する。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 新規作成日。
		 */
		union OptionValue
		{
			float number; //! Gauge の値。
			int index; //! Choice の番号。
		};
	}
}