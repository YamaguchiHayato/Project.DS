#include "stdafx.h"
#include "EnemyDeadState.h"

namespace nsApp
{
	namespace nsActor
	{
		EnEnemyState EnemyDeathState::GetStateKind() const
		{
			/* 樹の節は Death。*/
			return EnEnemyState::Death;
		}


		void EnemyDeathState::OnEnter()
		{
			/* 死亡時は待機表示のまま止める。*/
			pEnemy_->PlayIdle();
		}


		void EnemyDeathState::OnUpdate()
		{
			/* todo 死亡後は何もしない。*/
		}
	}
}