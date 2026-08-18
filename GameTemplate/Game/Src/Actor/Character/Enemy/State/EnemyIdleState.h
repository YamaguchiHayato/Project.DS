#pragma once
#include "Src/StateMachine/IState.h"
#include "Src/Actor/Actor.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnemyIdleState.h
		 * @brief  雑魚敵の待機ステート。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 */
		class EnemyIdleState : public nsState::IState<Actor>
		{
		public:
			/* ライフサイクル。*/
			/**
			 * @brief 待機開始。
			 */
			void Enter() override;

			/**
			 * @brief 待機更新。発見距離に入ったら追跡へ切り替える。
			 */
			void Update() override;

			/**
			 * @brief 待機終了。
			 */
			void Exit() override;
		};
	}
}