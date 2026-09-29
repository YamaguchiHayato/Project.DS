#pragma once
#include "EnemyStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnemyWanderState.h
		 * @brief  敵の徘徊ステート。
		 * @details 未発見時に Walk で短い距離を歩き、時間切れで Idle へ戻る。
		 *          流れは EnemyStateBase。差分は OnEnter / OnUpdate のみ。
		 * @author Yamaguchi Hayato
		 * @date   2026/09/24: 作成日。
		 */
		class EnemyWanderState : public EnemyStateBase
		{
		protected:
			/**
			 * @brief ステートの種類を返す。
			 * @return Wander。
			 */
			EnEnemyState GetStateKind() const override
			{
				return EnEnemyState::Wander;
			}

			/**
			 * @brief 徘徊開始。
			 */
			void OnEnter() override;

			/**
			 * @brief 徘徊を更新。
			 */
			void OnUpdate() override;
		};
	}
}
