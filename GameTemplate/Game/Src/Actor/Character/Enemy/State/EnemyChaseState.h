#pragma once
#include "Src/StateMachine/IState.h"
#include "Src/Actor/Actor.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
	     * @file   EnemyChaseState.h
		 * @brief  雑魚敵の追跡ステート。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 */
		class EnemyChaseState : public nsState::IState<Actor>
		{
		public:
			/* ライフサイクル。*/
			/**
			 * @brief 追跡開始。
			 */
			void Enter() override;

			/**
			 * @brief 追跡更新。対象へ近づく。
			 */
			void Update() override;

			/**
			 * @brief 追跡終了。
			 */
			void Exit() override;
		};
	}
}