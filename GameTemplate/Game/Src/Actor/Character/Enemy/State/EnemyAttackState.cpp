#include "stdafx.h"
#include "EnemyAttackState.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyChaseState.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemyAttackState::Enter()
		{
			/* 攻撃ステートに入ったらすぐ攻撃できるようにする。*/
			static_cast<CommonEnemy*>(pOwner_)->ReadyAttack();
		}


		void EnemyAttackState::Update()
		{
			/* 攻撃距離から出たら追跡へ戻す。*/
			if (!static_cast<CommonEnemy*>(pOwner_)->IsTargetInAttackRange())
			{
				pStateMachine_->ChangeState(new EnemyChaseState());
				return;
			}

			/* 対象を攻撃する。*/
			static_cast<CommonEnemy*>(pOwner_)->AttackTarget();
		}


		void EnemyAttackState::Exit()
		{
		}
	}
}