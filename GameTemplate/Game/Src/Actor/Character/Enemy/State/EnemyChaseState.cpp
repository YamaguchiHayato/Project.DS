#include "stdafx.h"
#include "EnemyChaseState.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyAttackState.h"
#include "Src/Actor/Character/Enemy/State/EnemyIdleState.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemyChaseState::Enter()
		{
			/* 歩きアニメを再生する。*/
			static_cast<CommonEnemy*>(pOwner_)->PlayWalk();
		}


		void EnemyChaseState::Update()
		{
			/* 追跡対象が死亡している場合は待機ステートに遷移する。*/
			CommonEnemy* pEnemy = static_cast<CommonEnemy*>(pOwner_);

			/* 追跡対象が攻撃距離に入っている場合は攻撃ステートに遷移する。*/
			if (pEnemy->IsTargetDead())
			{
				pStateMachine_->ChangeState(new EnemyIdleState());
				return;
			}

			/* 追跡対象が攻撃距離に入っている場合は攻撃ステートに遷移する。*/
			if (pEnemy->IsTargetInAttackRange())
			{
				pStateMachine_->ChangeState(new EnemyAttackState());
				return;
			}

			/* 追跡対象が発見距離から外れている場合は待機ステートに遷移する。*/
			pEnemy->MoveToTarget();

			/* 追跡対象が攻撃距離に入っている場合は攻撃ステートに遷移する。*/
			if(pEnemy->IsTargetInAttackRange())
				pStateMachine_->ChangeState(new EnemyAttackState());
		}


		void EnemyChaseState::Exit()
		{

		}
	}
}