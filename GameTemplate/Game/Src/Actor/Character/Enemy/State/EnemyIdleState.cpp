#include "stdafx.h"
#include "EnemyIdleState.h"

namespace nsApp
{
	namespace nsActor
	{
		EnEnemyState EnemyIdleState::GetStateKind() const
		{
			/* 樹の節は Idle。*/
			return EnEnemyState::Idle;
		}


		void EnemyIdleState::OnEnter()
		{
			/* 待機アニメを再生する。*/
			pEnemy_->PlayIdle();
		}


		void EnemyIdleState::OnUpdate()
		{
			/* TODO: 待機中の仕事は無い。遷移は基底に任せる。*/
		}
	}
}