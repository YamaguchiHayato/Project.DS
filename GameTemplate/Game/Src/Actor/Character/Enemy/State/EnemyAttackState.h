#pragma once
/**
 * @file   EnemyAttackState.h
 * @brief  雑魚敵の攻撃ステート。
 * @author Yamaguchi Hayato
 * @date   2026/08/18
 */

#include "Src/StateMachine/IState.h"
#include "Src/Actor/Actor.h"

namespace nsApp
{
	namespace nsActor
	{
		class EnemyAttackState : public nsState::IState<Actor>
		{
		public:
			/* ライフサイクル。*/
			/**
 			 * @brief 攻撃開始。
			 */
			void Enter() override;

			/**
			 * @brief 攻撃更新。距離が離れたら追跡へ戻る。
			 */
			void Update() override;

			/**
			 * @brief 攻撃終了。
			 */
			void Exit() override;
		};
	}
}