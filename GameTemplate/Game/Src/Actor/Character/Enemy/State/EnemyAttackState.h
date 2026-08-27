#pragma once
/**
 * @file   EnemyAttackState.h
 * @brief  敵の攻撃ステート。
 * @details IEnemy を所有者とする。流れは EnemyStateBase。差分は OnEnter / OnUpdate のみ。
 * @author Yamaguchi Hayato
 * @date   2026/08/18
 */

#include "Src/Actor/Character/Enemy/State/EnemyStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		class EnemyAttackState : public EnemyStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return Attack。
			 */
			EnEnemyState GetStateKind() const override;

			/**
			 * @brief 攻撃開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief 攻撃更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}