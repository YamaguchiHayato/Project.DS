#pragma once
#include "Src/Actor/Character/Enemy/State/EnemyStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnemyIdleState.h
		 * @brief  敵の待機ステート。
		 * @details IEnemy を所有者とする。流れは EnemyStateBase。差分は OnEnter / OnUpdate のみ。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 */
		class EnemyIdleState : public EnemyStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return Idle。
			 */
			EnEnemyState GetStateKind() const override;

			/**
			 * @brief 待機開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief 待機更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}