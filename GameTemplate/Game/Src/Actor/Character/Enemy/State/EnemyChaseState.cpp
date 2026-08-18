#include "stdafx.h"
#include "EnemyChaseState.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyAttackState.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemyChaseState::Enter()
		{
		}


		void EnemyChaseState::Update()
		{
			/* 対象へ近づく。*/
			static_cast<CommonEnemy*>(pOwner_)->MoveToTarget();

			/* 攻撃距離に入ったら攻撃へ切り替える。*/
			if (static_cast<CommonEnemy*>(pOwner_)->IsTargetInAttackRange())
				pStateMachine_->ChangeState(new EnemyAttackState());
		}


		void EnemyChaseState::Exit()
		{
		}
	}
}