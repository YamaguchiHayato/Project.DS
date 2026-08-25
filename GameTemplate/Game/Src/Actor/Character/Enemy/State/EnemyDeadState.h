#pragma once
#include "Src/StateMachine/IState.h"
#include "Src/Actor/Actor.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnemyDeathState.h
		 * @brief  雑魚敵の死亡ステート。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/20
		 */
		class EnemyDeathState : public nsState::IState<Actor>
		{
		public:
			/* ライフサイクル。*/
			/**
			 * @brief 死亡開始。
			 */
			void Enter() override;

			/**
			 * @brief 死亡更新。
			 */
			void Update() override;

			/**
			 * @brief 死亡終了。
			 */
			void Exit() override;
		};
	}
}