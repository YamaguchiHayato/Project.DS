#pragma once

#include "Src/Actor/Character/Enemy/Transition/EnEnemyState.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnemyTransitionEdge.h
		 * @brief  遷移樹の1本の枝。
		 * @details 「どの節から」「どんな事実が揃ったら」「どこへ行くか」を表す。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/26
		 */
		struct EnemyTransitionEdge
		{
			EnEnemyState enFrom_ = EnEnemyState::Idle; //! 出発するステート種別。
			uint32_t uNeedFacts_ = 0; //! 必要な事実（AND 条件）。
			EnEnemyState enTo_ = EnEnemyState::Idle; //! 遷移先のステート種別。
		};
	}
}