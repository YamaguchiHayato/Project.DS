#include "stdafx.h"
#include "PlayerItem.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Src/Actor/Character/Player/PlayerIntent.h"
#include "Src/Item/PipeBomb.h"
#include "Src/Item/Pickup.h"
#include "Weapon.h"

namespace
{
	const float MUZZLE_FORWARD = 50.0f;	//! 投擲始点を視線方向へずらす量。
	const float HEAL_EFFECT_HEIGHT = 80.0f;	//! 回復の演出を出す高さ(胸のあたり)。
}


namespace nsApp
{
	namespace nsActor
	{
		void PlayerItem::UpdateItems(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* メディキット(4/H長押し)。*/
			UpdateMedkit(fDeltaTime);

			/* メディキットを使っている間は他のアイテムに手が回らない。*/
			if (pPlayer_->IsHealing())
				return;

			/* 即効アイテム(5): 鎮痛剤・アドレナリンはその場で効く。*/
			if (pPlayer_->GetIntent().bQuickItemTrigger_)
				UseQuickItem();

			/* 投擲(パイプ爆弾): 1個消費して視線方向へ投げる。着地して敵を引き寄せてから爆発する。*/
			if (pPlayer_->GetIntent().bThrowTrigger_ && pPlayer_->GetPipeBombCount() > 0)
			{
				pPlayer_->AddPipeBombCount(-1);
				workLook_ = pPlayer_->GetLookDirection();
				workThrowPos_ = pPlayer_->GetEyePosition();
				workThrowPos_ += workLook_ * MUZZLE_FORWARD;
				NewGO<nsItem::PipeBomb>(0, "pipeBomb")->Setup(workThrowPos_, workLook_);
			}
		}


		void PlayerItem::UpdateMedkit(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 使える条件: 持っていて、恒久HPが減っている。*/
			workCanHeal_ = (pPlayer_->GetMedkitCount() > 0) && (pPlayer_->GetCurrentHP() < pPlayer_->GetMaxHP());

			/* 使う条件: 押し続けていて、動いておらず、撃ってもいない(手を離す・動く・撃つと中断)。*/
			workWantHeal_ = pPlayer_->GetIntent().bHealPress_ && !pPlayer_->IsMoving() && !pPlayer_->GetIntent().bFirePress_;

			/* 使えない・使わない場合は、回復中を解除して進捗をリセットする。*/
			if (!workCanHeal_ || !workWantHeal_)
			{
				pPlayer_->SetHealing(false);
				pPlayer_->SetMedkitProgress(0.0f);
				return;
			}

			/* 包帯を巻いている途中。アドレナリン中は手が速い。*/
			pPlayer_->SetHealing(true);
			pPlayer_->AddMedkitProgress(fDeltaTime * pPlayer_->GetActionSpeedRate() / pPlayer_->GetPlayerStatus().stHealthRule_.fMedkitUseTime_);
			if (pPlayer_->GetHealProgress() < 1.0f)
				return;

			/* 使い切った。失った恒久HPの一定割合を戻す(本家と同じ80%)。*/
			workMissing_ = pPlayer_->GetMaxHP() - pPlayer_->GetCurrentHP();
			workHeal_ = static_cast<int>(static_cast<float>(workMissing_) * pPlayer_->GetPlayerStatus().stHealthRule_.fMedkitHealRate_ + 0.5f);
			if (workHeal_ < 1)
				workHeal_ = 1;

			/* 恒久HPを回復する。*/
			pPlayer_->AddCurrentHP(workHeal_);

			/* 恒久HPが戻ったので一時体力は消え、復帰回数も数え直す(白黒が治る)。*/
			pPlayer_->SetTempHP(0.0f);
			pPlayer_->SetReviveCount(0);

			/* メディキットを1個消費して、回復中を解除し、進捗をリセットする。*/
			pPlayer_->AddMedkitCount(-1);
			pPlayer_->SetHealing(false);
			pPlayer_->SetMedkitProgress(0.0f);

			/* 回復演出を出す。*/
			workHealPos_ = pPlayer_->GetPosition();
			workHealPos_.y += HEAL_EFFECT_HEIGHT;
			pPlayer_->PublishGameEvent(nsEvent::EnGameEvent::PlayerHealed, workHealPos_, Vector3::Zero, workHeal_);
		}


		void PlayerItem::UseQuickItem()
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 何も持っていない場合は、処理しない。*/
			workGain_ = 0;
			switch (pPlayer_->GetQuickItem())
			{
			case EnQuickItem::Pills:
			{
				/* 満タンのときに飲んでも無駄になるだけなので、手元に残す。*/
				if (pPlayer_->GetTotalHP() >= pPlayer_->GetMaxHP())
					return;

				/* 一時体力を増やす。*/
				workGain_ = pPlayer_->GetPlayerStatus().stHealthRule_.iPillsTempHP_;
				break;
			}

			case EnQuickItem::Adrenaline:
			{
				/* 体力が満タンでも足の速さの効果があるので、いつでも使える。*/
				workGain_ = pPlayer_->GetPlayerStatus().stHealthRule_.iAdrenalineTempHP_;
				pPlayer_->SetAdrenalineTimer(pPlayer_->GetPlayerStatus().stHealthRule_.fAdrenalineTime_);
				break;
			}

			default: return;
			}

			/* 一時体力を増やす。*/
			pPlayer_->AddTempHP(workGain_);
			pPlayer_->SetQuickItem(EnQuickItem::None);

			/* 回復演出を出す。*/
			workHealPos_ = pPlayer_->GetPosition();
			workHealPos_.y += HEAL_EFFECT_HEIGHT;
			pPlayer_->PublishGameEvent(nsEvent::EnGameEvent::PlayerHealed, workHealPos_, Vector3::Zero, workGain_);
		}


		void PlayerItem::PickUpItem()
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 拾える距離にある物資を探す。*/
			for (nsItem::Pickup* pPickup : FindGOs<nsItem::Pickup>("pickup"))
			{
				if (pPickup == nullptr || !pPickup->IsInRange(pPlayer_->GetPosition()))
					continue;

				/*
				 * 弾薬の山は無くならない(本家と同じ)。所持している武器の予備弾を上限まで満たし、
				 * 補給できたら通知だけ出して終わる。満タンで補給できなければ次を探す。
				 */
				if (pPickup->GetType() == nsItem::EnPickupType::Ammo)
				{
					if (!pPlayer_->GetWeaponInventory().RefillReserveAmmoToAll())
						continue;

					pPlayer_->PublishGameEvent(nsEvent::EnGameEvent::ItemPickedUp, pPickup->GetPosition());
					return;
				}

				/* 種類に応じて手に入れる。満たされていて拾えない場合は次を探す。*/
				workPickedUp_ = false;
				switch (pPickup->GetType())
				{
				case nsItem::EnPickupType::Medkit:
					pPlayer_->AddMedkitCount(1);
					workPickedUp_ = true;
					break;

				case nsItem::EnPickupType::PipeBomb:
					pPlayer_->AddPipeBombCount(1);
					workPickedUp_ = true;
					break;

				case nsItem::EnPickupType::Pills:
					/* 即効アイテムは1つしか持てない。本家と同じく、持っているものと入れ替わる。*/
					pPlayer_->SetQuickItem(EnQuickItem::Pills);
					workPickedUp_ = true;
					break;

				case nsItem::EnPickupType::Adrenaline:
					pPlayer_->SetQuickItem(EnQuickItem::Adrenaline);
					workPickedUp_ = true;
					break;

				case nsItem::EnPickupType::Weapon:
					workPickedUp_ = SwapWeaponWithPickup(pPickup);
					break;

				default:
					break;
				}

				/* 拾えなければ物資はその場に残す。*/
				if (!workPickedUp_)
					continue;

				/* 拾ったことを通知して、その物資を消す。*/
				pPlayer_->PublishGameEvent(nsEvent::EnGameEvent::ItemPickedUp, pPickup->GetPosition());
				DeleteGO(pPickup);
				return;
			}
		}


		bool PlayerItem::SwapWeaponWithPickup(nsItem::Pickup* pPickup)
		{
			/* nullの場合、入れ替えない。*/
			if (pPlayer_ == nullptr || pPickup == nullptr)
				return false;

			/* 同じ区分の武器と入れ替える。すでに同じ銃を持っていれば何もしない。*/
			workOldWeaponType_ = nsWeapon::EnWeaponType::Handgun;
			if (!pPlayer_->GetWeaponInventory().ReplaceWeapon(pPickup->GetWeaponType(), workOldWeaponType_))
				return false;

			/*
			 * 手放した銃はその場に落とす(本家と同じく、拾った場所で入れ替わる)。
			 * 落とした銃は元の物資と同じ位置に置くので、拾い直せば元に戻せる。
			 */
			NewGO<nsItem::Pickup>(0, "pickup")->SetupWeapon(workOldWeaponType_, pPickup->GetPosition());

			return true;
		}
	}
}