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
			/* 待機アニメを再生する。*/
			static_cast<CommonEnemy*>(pOwner_)->PlayIdle();
		}


		void EnemyIdleState::Update()
		{
			/* オーナーをキャストする。*/
			CommonEnemy* pEnemy = static_cast<CommonEnemy*>(pOwner_);

			/* 対象が死亡していれば待機のまま。*/
			if (pEnemy->IsTargetDead())
				return;

			/* 発見距離の外なら待機のまま。*/
			if (!pEnemy->IsTargetInDetectRange())
				return;

			/* 視線が通っていなければ待機のまま。*/
			if (!pEnemy->IsTargetVisible())
				return;

			/* 距離内かつ視線ありなら追跡へ。*/
			pStateMachine_->ChangeState(new EnemyChaseState());
		}


		void EnemyIdleState::Exit()
		{
		}
	}
}