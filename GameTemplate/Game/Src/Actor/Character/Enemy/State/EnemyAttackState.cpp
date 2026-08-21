#include "stdafx.h"
#include "EnemyAttackState.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyChaseState.h"
#include "Src/Actor/Character/Enemy/State/EnemyIdleState.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemyAttackState::Enter()
		{
			/* 攻撃ステートに入ったらすぐ攻撃できるようにする。*/
			static_cast<CommonEnemy*>(pOwner_)->ReadyAttack();

			/* 攻撃アニメを再生する。*/
			static_cast<CommonEnemy*>(pOwner_)->PlayIdle();
		}


		void EnemyAttackState::Update()
		{
			/* 対象が死亡していれば待機へ戻す。*/
			if (static_cast<CommonEnemy*>(pOwner_)->IsTargetDead())
			{
				pStateMachine_->ChangeState(new EnemyIdleState());
				return;
			}

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