#include "stdafx.h"
#include "PlayerAttackState.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Weapon.h"

namespace nsApp
{
	namespace nsActor
	{
		//! static 変数を初期化。
		AttackFireStrategy PlayerAttackState::fireStrategy_; 
		AttackReloadStrategy PlayerAttackState::reloadStrategy_; 
		AttackSwitchStrategy PlayerAttackState::switchStrategy_; 
		AttackShoveStrategy PlayerAttackState::shoveStrategy_; 
		AttackAdsStrategy PlayerAttackState::adsStrategy_;

		EnPlayerState PlayerAttackState::GetStateKind() const
		{
			/* 樹の節は Attack。*/
			return EnPlayerState::Attack;
		}


		void PlayerAttackState::OnEnter()
		{
			/*
			 * 銃戦闘ステートに入ったときの準備をここに書く。
			 * Attack.tka は再生しない（銃は Idle/Walk/Run の上で扱う）。
			 */
		}


		void PlayerAttackState::OnUpdate()
		{
			/* 銃戦闘ステートの更新処理。Player::Update() から呼ばれる。*/
			const auto DELTATIME = g_gameTime->GetFrameDeltaTime();
			TickWeapon(pPlayer_, DELTATIME);
		}


		void PlayerAttackState::TickWeapon(Player* pPlayer, float deltaTime)
		{
			/* null なら処理を止める。*/
			if (pPlayer == nullptr)
				return;

			/* 武器のクールタイム・リロード進行などを進める。アドレナリン中は手が速い。*/
			pPlayer->GetWeaponInventory().Update(deltaTime, pPlayer->GetActionSpeedRate());

			/* 覗き込みは Strategy に任せる。*/
			adsStrategy_.Execute(pPlayer, deltaTime);

			/* メディキット使用中は両手が塞がる。撃つ・リロード・持ち替えはできない。*/
			if (!pPlayer->IsHealing())
			{
				/* リロードは Strategy に任せる。*/
				reloadStrategy_.Execute(pPlayer, deltaTime);

				/* 発射は Strategy に任せる（武器差はデータ側）。*/
				fireStrategy_.Execute(pPlayer, deltaTime);

				/* 持ち替えは Strategy に任せる。*/
				switchStrategy_.Execute(pPlayer, deltaTime);
			}

			/* 突き飛ばしは Strategy に任せる。*/
			shoveStrategy_.Execute(pPlayer, deltaTime);
		}
	}
}