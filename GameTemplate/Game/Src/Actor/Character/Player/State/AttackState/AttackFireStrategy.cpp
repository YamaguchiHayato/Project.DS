#include "stdafx.h"
#include "AttackFireStrategy.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Weapon.h"

namespace
{
	const float MUZZLE_FORWARD = 50.0f;
	const float MUZZLE_RIGHT = 40.0f;
	const float MUZZLE_DOWN = -32.0f;
}

namespace nsApp
{
	namespace nsActor
	{
		void AttackFireStrategy::Execute(Player* pPlayer, float deltaTime)
		{
			/*  null なら実装しない。*/
			if (pPlayer == nullptr)
				return;

			/* 照準方向を取得する。*/
			workAimDir_ = pPlayer->GetLookDirection();

			/* 武器を持っていなければ撃てない。*/
			if (pPlayer->GetWeaponInventory().GetCurrentWeapon() == nullptr)
				return;

			/* 射撃したいかを取得する。フルオートならボタン押しっぱなし、セミオートならトリガーを引いた瞬間だけ。*/
			workWantFire_ = pPlayer->GetWeaponInventory().GetCurrentWeapon()->IsFullAuto()? pPlayer->GetIntent().bFirePress_ : pPlayer->GetIntent().bFireTrigger_;

			/* バースト射撃中ならボタンを離しても撃ち続ける。*/
			if (pPlayer->GetWeaponInventory().GetCurrentWeapon()->IsBurstPending())
				workWantFire_ = true;

			/* 射撃したいけど弾が無い場合はリロードする。*/
			if (!workWantFire_)
				return;

			/*
			 * この1発がどちらの銃口から出るかを決める。
			 * 二丁持ちは撃つたびに左右が入れ替わるので、横へずらす向きもそれに合わせる。
			 * 1丁の銃は常に右のまま。
			 */
			workMuzzleSideSign_ = pPlayer->GetWeaponInventory().GetCurrentWeapon()->IsNextMuzzleLeft() ? -1.0f : 1.0f;

			/* 銃口の位置を計算する。目の位置から左右にずらして、照準方向に前方へずらす。さらに下にずらす。*/
			workEyePos_ = pPlayer->GetEyePosition();
			workMuzzleRight_ = { cosf(pPlayer->GetCameraYaw()), 0.0f, -sinf(pPlayer->GetCameraYaw()) };
			workMuzzlePos_ = workEyePos_;
			workMuzzlePos_ += workMuzzleRight_ * (MUZZLE_RIGHT * workMuzzleSideSign_);
			workMuzzlePos_ += workAimDir_ * MUZZLE_FORWARD;
			workMuzzlePos_.y += MUZZLE_DOWN;

			/* 射撃する。弾が無ければ撃てない。*/
			if (!pPlayer->GetWeaponInventory().Fire(workMuzzlePos_, workAimDir_))
				return;

			/* 射撃の反動を加える。*/
			pPlayer->ApplyFireRecoil(pPlayer->GetWeaponInventory().GetCurrentWeapon());
			pPlayer->AddSpreadShot(pPlayer->GetWeaponInventory().GetCurrentWeapon()->GetSpreadPerShot());

			/* 拡散が最大値を超えないようにする。*/
			if (pPlayer->GetSpreadShot() > pPlayer->GetWeaponInventory().GetCurrentWeapon()->GetMaxSpreadShot())
				pPlayer->SetSpreadShot(pPlayer->GetWeaponInventory().GetCurrentWeapon()->GetMaxSpreadShot());

			/* 射撃イベントを発行する。*/
			pPlayer->PublishGameEvent(nsEvent::EnGameEvent::WeaponFired, workMuzzlePos_, workAimDir_);

			/* 弾を撃つ。散弾は複数回呼ぶ。*/
			for (workPelletIndex_ = 0; workPelletIndex_ < pPlayer->GetWeaponInventory().GetCurrentWeapon()->GetPelletCount(); workPelletIndex_++)
				pPlayer->FireHitScan(pPlayer->GetWeaponInventory().GetCurrentWeapon(), workEyePos_, workMuzzlePos_, workAimDir_);
		}
	}
}