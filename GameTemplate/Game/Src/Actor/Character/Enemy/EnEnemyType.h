#pragma once
#include "stdint.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnEnemyType.h
		 * @brief  ゾンビの種類。
		 * @details モデルの種類(CharacterModelType)とは別に、
		 *          「どのゾンビを湧かせるか」を表すラベルとして使う。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/06: 新規作成日。
		 */
		enum class EnEnemyType : uint8_t
		{
			None,   //! 未設定。スポナーは何も湧かせない。
			Common,	//! 雑魚ゾンビ。
			Bile,   //! ゲロ。
			Bomber, //! 爆弾ゾンビ。
		};
	}
}