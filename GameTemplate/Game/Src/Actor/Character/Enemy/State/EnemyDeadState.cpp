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
			pEnemy_->PlayDeath();
		}


		void EnemyDeathState::OnUpdate()
		{
			/* 死亡演出を更新する。*/
			pEnemy_->ExecuteDeth();
		}
	}
}

