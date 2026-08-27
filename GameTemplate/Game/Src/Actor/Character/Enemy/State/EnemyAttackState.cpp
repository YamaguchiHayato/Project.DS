#include "stdafx.h"
#include "EnemyAttackState.h"

namespace nsApp
{
	namespace nsActor
	{
		EnEnemyState EnemyAttackState::GetStateKind() const
		{
			/* 樹の節は Attack。*/
			return EnEnemyState::Attack;
		}


		void EnemyAttackState::OnEnter()
		{
			/* 攻撃ステートに入ったらすぐ攻撃できるようにする。*/
			pEnemy_->ReadyAttack();

			/* 攻撃アニメを再生する。*/
			pEnemy_->PlayIdle();
		}


		void EnemyAttackState::OnUpdate()
		{
			/* 対象を攻撃する。*/
			pEnemy_->AttackTarget();
		}
	}
}