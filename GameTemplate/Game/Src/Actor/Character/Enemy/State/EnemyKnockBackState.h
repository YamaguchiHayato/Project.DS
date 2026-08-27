#pragma once
#include "Src/Actor/Character/Enemy/State/EnemyStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnemyKnockBackState.h
		 * @brief  敵のノックバックステート。
		 * @details IEnemy を所有者とする。流れは EnemyStateBase。差分は OnEnter / OnUpdate のみ。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/26
		 */
		class EnemyKnockBackState : public EnemyStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return KnockBack。
			 */
			EnEnemyState GetStateKind() const override;

			/**
			 * @brief ノックバック開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief ノックバック更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}