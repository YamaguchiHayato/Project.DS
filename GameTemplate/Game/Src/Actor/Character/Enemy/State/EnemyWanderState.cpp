#include "stdafx.h"
#include "EnemyWanderState.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemyWanderState::OnEnter()
		{
			/* 徘徊する向きを初期化する。*/
			pEnemy_->BeginWander();

			/* 徘徊アニメーションを適応する。*/
			pEnemy_->PlayWalk();
		}


		void EnemyWanderState::OnUpdate()
		{
			/* 方向を決めて徘徊をする。*/
			pEnemy_->ExecuteWander();
		}
	}
}