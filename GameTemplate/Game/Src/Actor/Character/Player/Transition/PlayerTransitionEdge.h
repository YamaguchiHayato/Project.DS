#pragma once

#include "EnPlayerState.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerTransitionEdge.h
		 * @brief  遷移樹の1本の枝。
		 * @details 「どの節から」「どんな事実が揃ったら」「どこへ行くか」を表す。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16: 新規作成日。
		 */
		struct PlayerTransitionEdge
		{
			EnPlayerState enFrom_ = EnPlayerState::Idle; //! 出発するステート種別。
			uint32_t needFacts_ = 0; //! 必要な事実（AND 条件）。
			EnPlayerState enTo_ = EnPlayerState::Idle; //! 遷移先のステート種別。
		};
	}
}