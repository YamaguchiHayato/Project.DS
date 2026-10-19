#include "stdafx.h"
#include "PlayerCombat.h"
#include "Player.h"
#include "PlayerIntent.h"
#include "Weapon.h"

/* todo 敵全体をターゲットにできるように修正。*/
#include "Src/Actor/Character/Enemy/CommonEnemy.h"

namespace
{
	const float MUZZLE_FORWARD = 50.0f;	//! 銃口の前方オフセット(照準方向)。
	const float MUZZLE_RIGHT = 40.0f;	//! トレーサー始点を目線から右へずらす量(線を斜めに見せる)。
	const float MUZZLE_DOWN = -32.0f;	//! トレーサー始点を目線から下へずらす量(負で下)。
	const float ENEMY_CENTER_HEIGHT_FOR_SHOVE = 85.0f;	//! 突き飛ばしの手応えを出す高さ(敵の体の中心)。
	const int SHOVE_DAMAGE = 10;			//! 突き飛ばしで与えるダメージ。押し返しが主で、削るのはおまけ。
}


namespace nsApp
{
	namespace nsActor
	{
		void PlayerCombat::UpdateWeapon(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 武器のクールタイム・リロード等を進める。アドレナリン中は手が速い。*/
			pPlayer_->GetWeaponInventory().Update(fDeltaTime, pPlayer_->GetActionSpeedRate());

			/* メディキットを使っている間は両手が塞がっている。撃つ・リロード・持ち替えはできない。*/
			if (pPlayer_->IsHealing())
				return;

			/* リロード。*/
			if (pPlayer_->GetIntent().bReloadTrigger_)
				pPlayer_->GetWeaponInventory().Reload();

			/* 照準方向はカメラの視線(ヨー＋ピッチ。画面中央=クロスヘア方向)。*/
			workAimDir_ = pPlayer_->GetLookDirection();

			/* 現在の武器に応じて、発射入力を押しっぱなし(フルオート)か押した瞬間(単発)で判定する。*/
			if (pPlayer_->GetWeaponInventory().GetCurrentWeapon() != nullptr)
			{
				/* フルオートなら押しっぱなし、単発なら押した瞬間で撃つ。*/
				workWantFire_ = pPlayer_->GetWeaponInventory().GetCurrentWeapon()->IsFullAuto() ? pPlayer_->GetIntent().bFirePress_	: pPlayer_->GetIntent().bFireTrigger_;

				/* 点射の残りは、引き金を離していても撃ち切る。*/
				if (pPlayer_->GetWeaponInventory().GetCurrentWeapon()->IsBurstPending())
					workWantFire_ = true;

				/* 発射する場合は、ヒットスキャン判定＋トレーサー表示を行う。*/
				if (workWantFire_)
				{
					/* 命中判定はカメラ(目)=クロスヘアから飛ばす。銃口はトレーサーの見た目始点にだけ使う。*/
					workEyePos_ = pPlayer_->GetEyePosition();
					/* トレーサーの始点=画面の銃口あたり(目線から右・下・前へずらす)。
					   目線=射線と横にずらすことで、線が点に潰れず斜めの線として見える。*/
					workMuzzleRight_ = { cosf(pPlayer_->GetCameraYaw()), 0.0f, -sinf(pPlayer_->GetCameraYaw()) };
					workMuzzlePos_ = workEyePos_;
					workMuzzlePos_ += workMuzzleRight_ * MUZZLE_RIGHT;
					workMuzzlePos_ += workAimDir_ * MUZZLE_FORWARD;
					workMuzzlePos_.y += MUZZLE_DOWN;

					/* 発射に成功したら、ヒットスキャン判定＋トレーサー表示を行う。*/
					if (pPlayer_->GetWeaponInventory().Fire(workMuzzlePos_, workAimDir_))
					{
						/* 発射の反動を加える(視点が跳ね、銃が手前へ下がる)。*/
						pPlayer_->ApplyFireRecoil(pPlayer_->GetWeaponInventory().GetCurrentWeapon());

						/* 連射するほど弾がばらつくようにする。*/
						pPlayer_->AddSpreadShot(pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetSpreadPerShot());

						/* 散弾の粒数が多い武器は、1発撃つと散弾の粒数分だけばらつきが増える。*/
						if (pPlayer_->GetSpreadShot() > pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetMaxSpreadShot())
							pPlayer_->SetSpreadShot(pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetMaxSpreadShot());

						/* 発射したことを通知する(演出は購読側が担当する)。*/
						pPlayer_->PublishGameEvent(nsEvent::EnGameEvent::WeaponFired, workMuzzlePos_, workAimDir_);

						/* 弾を飛ばす。散弾は1発で複数の粒が別々の向きへ飛ぶので、粒の数だけ判定する。*/
						for (workPelletIndex_ = 0; workPelletIndex_ < pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetPelletCount(); workPelletIndex_++)
							pPlayer_->FireHitScan(pPlayer_->GetWeaponInventory().GetCurrentWeapon(), workEyePos_, workMuzzlePos_, workAimDir_);
					}
				}
			}

			/* ダウン中はサブ武器で固定。持ち替えられるのは立っているときだけ。*/
			if (pPlayer_->GetLifeState() != EnLifeState::Alive)
				return;

			/* 数字キーでの直接持ち替え(ホイールより優先する)。*/
			if (pPlayer_->GetIntent().bMainWeaponTrigger_)
				pPlayer_->GetWeaponInventory().SwitchToSlot(nsWeapon::EnWeaponSlot::Main);
			else if (pPlayer_->GetIntent().bSubWeaponTrigger_)
				pPlayer_->GetWeaponInventory().SwitchToSlot(nsWeapon::EnWeaponSlot::Sub);

			/* 武器切り替え(マウスホイール)。*/
			if (pPlayer_->GetIntent().bWeaponNextTrigger_)
				pPlayer_->GetWeaponInventory().SwitchNext();
			else if (pPlayer_->GetIntent().bWeaponPrevTrigger_)
				pPlayer_->GetWeaponInventory().SwitchPrev();
		}


		void PlayerCombat::UpdateShove(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* クールダウンを進める。*/
			if (pPlayer_->GetShoveCooldown() > 0.0f)
				pPlayer_->AddShoveCooldown(-fDeltaTime);

			/* 押さずにいた時間で疲労が1回ぶんずつ抜けていく。*/
			pPlayer_->AddShoveRestTimer(fDeltaTime);
			while (pPlayer_->GetShoveFatigue() > 0 && pPlayer_->GetShoveRestTimer() >= pPlayer_->GetPlayerStatus().fShoveFatigueRecoverTime_)
			{
				pPlayer_->AddShoveStack(-1);
				pPlayer_->AddShoveRestTimer(-pPlayer_->GetPlayerStatus().fShoveFatigueRecoverTime_);
			}

			/* 突き出した銃を戻す。*/
			workMotionRate_ = fDeltaTime * pPlayer_->GetPlayerStatus().fShoveMotionRecoverRate_;
			if (workMotionRate_ > 1.0f)
				workMotionRate_ = 1.0f;

			pPlayer_->AddShoveMotion(-pPlayer_->GetShoveMotion() * workMotionRate_);

			/* 突き飛ばし入力が無い、またはクールダウン中なら何もしない。*/
			if (!pPlayer_->GetIntent().bShoveTrigger_ || pPlayer_->GetShoveCooldown() > 0.0f)
				return;

			/* メディキットを使っている間は両手が塞がっていて押せない。*/
			if (pPlayer_->IsHealing())
				return;

			/*
			 * 続けて押すと疲れて、次までの間が長くなる(本家の押し返し疲労)。
			 * 押し続けるだけで安全にならないようにするため。アドレナリン中は疲れない。
			 */
			pPlayer_->AddShoveStack(1);
			pPlayer_->SetShoveRestTimer(0.0f);
			workIsFatigued_ = !pPlayer_->IsAdrenalineActive() && (pPlayer_->GetShoveFatigue() >= pPlayer_->GetPlayerStatus().iShoveFatigueCount_);
			pPlayer_->SetShoveCooldown(workIsFatigued_ ? pPlayer_->GetPlayerStatus().fShoveFatigueCooldownTime_ : pPlayer_->GetPlayerStatus().fShoveCooldownTime_);

			/* 銃を前へ突き出す見た目。*/
			pPlayer_->SetShoveMotion(1.0f);

			/* 手を使うので、途中だったリロードは中断される(本家と同じ)。*/
			pPlayer_->GetWeaponInventory().CancelReload();

			/* 正面(水平)方向。*/
			workAimDir_ = { sinf(pPlayer_->GetCameraYaw()), 0.0f, cosf(pPlayer_->GetCameraYaw()) };

			/* 前方の近距離にいる敵を押し返す。*/
			for (CommonEnemy* pEnemy : FindGOs<CommonEnemy>("commonEnemy"))
			{
				if (pEnemy == nullptr)
					continue;

				/* プレイヤーから敵への水平ベクトル。*/
				workToEnemy_ = pEnemy->GetPosition() - pPlayer_->GetPosition();
				workToEnemy_.y = 0.0f;
				workDist_ = workToEnemy_.Length();

				/* 近距離かつ正面(約120度以内)のみ対象。*/
				if (workDist_ <= 0.0001f || workDist_ > pPlayer_->GetPlayerStatus().fShoveRange_)
					continue;

				workDir_ = workToEnemy_;
				workDir_.Normalize();
				if (workDir_.Dot(workAimDir_) < pPlayer_->GetPlayerStatus().fShoveFrontDot_)
					continue;

				/*
				 * 敵を外側へ押し返す(のけぞりの簡易版)。
				 * 座標を直接書き換えるだけだと敵の移動処理に上書きされてしまうため、
				 * 移動処理へも反映される SetPosition を使う。
				 */
				pEnemy->SetPosition(pEnemy->GetPosition() + workDir_ * pPlayer_->GetPlayerStatus().fShovePush_);

				/* 押し返すだけでなく、わずかに削る。*/
				pEnemy->ApplyDamage(SHOVE_DAMAGE);

				/* 当たった手応えを通知する(位置は敵の体の中心あたり)。*/
				workShoveHitPos_ = pEnemy->GetPosition();
				workShoveHitPos_.y += ENEMY_CENTER_HEIGHT_FOR_SHOVE;
				pPlayer_->PublishGameEvent(nsEvent::EnGameEvent::BulletHit, workShoveHitPos_, workDir_, SHOVE_DAMAGE);

				/* 倒したら退場させる。*/
				if (pEnemy->IsDead())
				{
					pPlayer_->PublishGameEvent(nsEvent::EnGameEvent::EnemyKilled, pEnemy->GetPosition());
					DeleteGO(pEnemy);
				}
			}

			/* TODO: 突き飛ばしのSE/モーション、特殊感染者への効果差など。*/
			OutputDebugStringA("[Player] Shove!\n");
		}


		void PlayerCombat::UpdateAction()
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* インタラクト: 足元に落ちている物資を拾う。*/
			if (pPlayer_->GetIntent().bUseTrigger_)
				pPlayer_->PickUpItem();

			/* ライトのON/OFF切り替え。*/
			if (pPlayer_->GetIntent().bLightTrigger_)
				pPlayer_->ToggleLightOn();

			/* メニュー・ポーズ画面。*/
			if (pPlayer_->GetIntent().bPauseTrigger_)
			{
				/* TODO: ポーズ画面/メニューを開く処理に差し替える。*/
				OutputDebugStringA("[Player] Pause!\n");
			}
		}
	}
}