#include "stdafx.h"
#include "EnemyKnockBackState.h"

namespace nsApp
{
	namespace nsActor
	{
		EnEnemyState EnemyKnockBackState::GetStateKind() const
		{
			/* 樹の節は KnockBack。*/
			return EnEnemyState::KnockBack;
		}


		void EnemyKnockBackState::OnEnter()
		{
			/* ノックバック用の速度・タイマーを用意する。*/
			pEnemy_->BeginKnockBack();

			/* 仮の見た目は待機。*/
			pEnemy_->PlayIdle();
		}


		void EnemyKnockBackState::OnUpdate()
		{
			/* 後ずさり／吹き飛びを進める。終了判定もここで行う。*/
			pEnemy_->ExecuteKnockBack();
		}
	}
}