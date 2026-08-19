#include "stdafx.h"
#include "EnemyIdleState.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyChaseState.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemyIdleState::Enter()
		{
		}


		void EnemyIdleState::Update()
		{
			/* 対象が死亡していれば待機のまま。*/
			if (static_cast<CommonEnemy*>(pOwner_)->IsTargetDead())
				return;

			/* 発見距離に入ったら追跡へ切り替える。*/
			if (static_cast<CommonEnemy*>(pOwner_)->IsTargetInDetectRange())
				pStateMachine_->ChangeState(new EnemyChaseState());
		}


		void EnemyIdleState::Exit()
		{
		}
	}
}