#include "stdafx.h"
#include "EnemyChaseState.h"

namespace nsApp
{
	namespace nsActor
	{
		EnEnemyState EnemyChaseState::GetStateKind() const
		{
			/* 樹の節は Chase。*/
			return EnEnemyState::Chase;
		}


		void EnemyChaseState::OnEnter()
		{
			/* 歩きアニメを再生する。*/
			pEnemy_->PlayRun();
		}


		void EnemyChaseState::OnUpdate()
		{
			/* 対象へ近づく。*/
			pEnemy_->MoveToTarget();
		}
	}
}