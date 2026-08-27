#pragma once
#include "Src/Actor/Character/Enemy/State/EnemyStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnemyChaseState.h
		 * @brief  敵の追跡ステート。
		 * @details IEnemy を所有者とする。流れは EnemyStateBase。差分は OnEnter / OnUpdate のみ。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 */
		class EnemyChaseState : public EnemyStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return Chase。
			 */
			EnEnemyState GetStateKind() const override;

			/**
			 * @brief 追跡開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief 追跡更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}