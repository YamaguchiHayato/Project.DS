#include "stdafx.h"
#include "AttackReloadStrategy.h"
#include "Src/Actor/Character/Player/Player.h"

namespace nsApp
{
	namespace nsActor
	{
		void AttackReloadStrategy::Execute(Player* pPlayer, float deltaTime)
		{
			/* 所有者がいなければ何もしない。*/
			if (pPlayer == nullptr)
				return;

			/* リロード入力が無ければ何もしない。*/
			if (!pPlayer->GetIntent().bReloadTrigger_)
				return;

			/* インベントリにリロード開始を依頼する（進行は Inventory.Update 側）。*/
			pPlayer->GetWeaponInventory().Reload();
		}
	}
}