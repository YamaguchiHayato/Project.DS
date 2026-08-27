#pragma once
#include "stdint.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnEnemyState.h
		 * @brief  ゾンビ共通のステート種別。
		 * @details State 実体（EnemyIdleState など）とは別に、
		 *          「今どの種類のステートか」を表すラベルとして使う。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/26
		 */
		enum class EnEnemyState : uint8_t
		{
			Idle,		//! 待機（または将来の徘徊待ち）。
			Chase,		//! 追跡。
			Attack,		//! 攻撃。
			Death,		//! 死亡。
			KnockBack,	//! ヒットによる後ずさり／吹き飛び。
		};
	}
}