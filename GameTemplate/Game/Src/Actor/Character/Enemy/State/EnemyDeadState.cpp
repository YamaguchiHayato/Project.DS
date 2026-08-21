#include "stdafx.h"
#include "EnemyDeadState.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemyDeathState::Enter()
		{
			/* 死亡時は待機表示のまま止める。*/
			static_cast<CommonEnemy*>(pOwner_)->PlayIdle();
		}


		void EnemyDeathState::Update()
		{
			/* 死亡後は何もしない。*/

		}


		void EnemyDeathState::Exit()
		{
		}
	}
}