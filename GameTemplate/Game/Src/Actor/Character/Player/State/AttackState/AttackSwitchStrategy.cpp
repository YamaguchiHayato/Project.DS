#include "stdafx.h"
#include "AttackSwitchStrategy.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Weapon.h"

namespace nsApp
{
	namespace nsActor
	{
		void AttackSwitchStrategy::Execute(Player* pPlayer, float deltaTime)
		{
			/* 所有者がいなければ何もしない。*/
			if (pPlayer == nullptr)
				return;

			/* ダウン中はサブ固定。持ち替えは立っているときだけ。*/
			if (pPlayer->GetLifeState() != EnLifeState::Alive)
				return;

			/* 数字キーでの直接持ち替え（ホイールより優先）。*/
			if (pPlayer->GetIntent().bMainWeaponTrigger_)
				pPlayer->GetWeaponInventory().SwitchToSlot(nsWeapon::EnWeaponSlot::Main);
			else if (pPlayer->GetIntent().bSubWeaponTrigger_)
				pPlayer->GetWeaponInventory().SwitchToSlot(nsWeapon::EnWeaponSlot::Sub);

			/* マウスホイールでの武器切り替え。*/
			if (pPlayer->GetIntent().bWeaponNextTrigger_)
				pPlayer->GetWeaponInventory().SwitchNext();
			else if (pPlayer->GetIntent().bWeaponPrevTrigger_)
				pPlayer->GetWeaponInventory().SwitchPrev();
		}
	}
}