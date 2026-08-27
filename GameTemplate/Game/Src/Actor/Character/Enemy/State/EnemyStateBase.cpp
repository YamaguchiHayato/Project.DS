#include "stdafx.h"
#include "EnemyStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemyStateBase::Enter()
		{
			/* 種類に依存しない口へ、入室時に一度だけ寄せる。*/
			pEnemy_ = static_cast<IEnemy*>(pOwner_);

			/* 樹の現在節をこのステート種別に同期する。*/
			pEnemy_->NotifyEnemyState(GetStateKind());

			/* ステート固有の入室処理。*/
			OnEnter();
		}


		void EnemyStateBase::Update()
		{
			/* 遷移が必要なら切り替えて、このフレームの仕事はしない。*/
			if (pEnemy_->TryChangeState())
				return;

			/* ステート固有の仕事。*/
			OnUpdate();
		}


		void EnemyStateBase::Exit()
		{
			/* 所有者キャッシュを捨てる。*/
			pEnemy_ = nullptr;
		}
	}
}