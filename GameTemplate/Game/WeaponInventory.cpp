#include "stdafx.h"
#include "WeaponInventory.h"
#include "Src/Data/WeaponStatusTable.h"

namespace nsApp
{
	namespace nsWeapon
	{
		void WeaponInventory::AddWeapon(EnWeaponType enType)
		{
			/* その武器が本来入る枠を、武器データから引く。*/
			const EnWeaponSlot enSlot = nsData::WeaponStatusTable::Get(enType).enSlot_;
			const int iSlot = static_cast<int>(enSlot);

			/* 1丁目かどうかは、入れる前に見ておく。*/
			const bool bWasEmpty = IsEmpty();

			/* 枠へ入れる。すでに何か入っていれば上書きする。*/
			aWeapons_[iSlot].Init(enType);
			abHasWeapon_[iSlot] = true;

			/* 最初の1丁はそのまま構えた状態にしておく。*/
			if (bWasEmpty)
				enCurrentSlot_ = enSlot;
		}


		bool WeaponInventory::ReplaceWeapon(EnWeaponType enNewType, EnWeaponType& enOutOldType)
		{
			/* 入れ先の枠を決める。*/
			const EnWeaponSlot enSlot = DecideSlot(enNewType);

			/* 入れ先が無ければ拾わない。*/
			if (enSlot == EnWeaponSlot::Num)
				return false;

			const int iSlot = static_cast<int>(enSlot);

			/*
			 * 手放す銃を呼び出し側へ返す。
			 * 空いていた枠へ入れた場合は何も手放していないので、落とす銃は無いことを伝える。
			 */
			enOutOldType = abHasWeapon_[iSlot] ? aWeapons_[iSlot].GetType() : EnWeaponType::Num;

			/* 入れ替えて、その銃を構える。*/
			aWeapons_[iSlot].Init(enNewType);
			abHasWeapon_[iSlot] = true;
			EquipSlot(enSlot);

			/* 同じ銃が両方の枠に揃ったら、二丁持ちへまとめる。*/
			TryMergeDualWeapon();

			return true;
		}


		void WeaponInventory::Update(float fDeltaTime, float fActionSpeedRate)
		{
			/* 現在の武器だけ更新する。*/
			Weapon* pCurrent = GetCurrentWeapon();
			if (pCurrent != nullptr)
				pCurrent->Update(fDeltaTime, fActionSpeedRate);
		}


		bool WeaponInventory::Fire(const Vector3& vPosition, const Vector3& vDirection)
		{
			/* 現在の武器で発射する。*/
			Weapon* pCurrent = GetCurrentWeapon();
			if (pCurrent == nullptr)
				return false;

			return pCurrent->Fire(vPosition, vDirection);
		}


		void WeaponInventory::Reload()
		{
			/* 現在の武器をリロードする。*/
			Weapon* pCurrent = GetCurrentWeapon();
			if (pCurrent != nullptr)
				pCurrent->Reload();
		}


		void WeaponInventory::CancelReload()
		{
			/* 現在の武器のリロードを止める。*/
			Weapon* pCurrent = GetCurrentWeapon();
			if (pCurrent != nullptr)
				pCurrent->CancelReload();
		}


		void WeaponInventory::SwitchNext()
		{
			/* 今の枠の次から順に、持っている枠を探す。*/
			for (int i = 1; i <= iSlotNum_; i++)
			{
				const int iSlot = (static_cast<int>(enCurrentSlot_) + i) % iSlotNum_;

				/* 持っていない枠は飛ばす。*/
				if (!abHasWeapon_[iSlot])
					continue;

				/* 1丁しか持っていなければ今の枠へ戻ってくる。構え直しは起こさない。*/
				if (static_cast<EnWeaponSlot>(iSlot) == enCurrentSlot_)
					return;

				EquipSlot(static_cast<EnWeaponSlot>(iSlot));
				return;
			}
		}


		void WeaponInventory::SwitchPrev()
		{
			/* 今の枠の手前から順に、持っている枠を探す。*/
			for (int i = 1; i <= iSlotNum_; i++)
			{
				const int iSlot = (static_cast<int>(enCurrentSlot_) + iSlotNum_ - (i % iSlotNum_)) % iSlotNum_;

				/* 持っていない枠は飛ばす。*/
				if (!abHasWeapon_[iSlot])
					continue;

				/* 1丁しか持っていなければ今の枠へ戻ってくる。構え直しは起こさない。*/
				if (static_cast<EnWeaponSlot>(iSlot) == enCurrentSlot_)
					return;

				EquipSlot(static_cast<EnWeaponSlot>(iSlot));
				return;
			}
		}


		void WeaponInventory::SwitchToSlot(EnWeaponSlot enSlot)
		{
			/* どの枠でもない指定は無視する。*/
			if (enSlot == EnWeaponSlot::Num)
				return;

			/* 持っていない区分へは持ち替えられない。*/
			if (!abHasWeapon_[static_cast<int>(enSlot)])
				return;

			/* すでに構えているなら持ち替えない(構え直しを起こさない)。*/
			if (enSlot == enCurrentSlot_)
				return;

			EquipSlot(enSlot);
		}


		bool WeaponInventory::AddReserveAmmoToAll(int iAmount)
		{
			/* 1挺でも補給できたかを覚えておく。*/
			bool bAdded = false;

			/* 持っている枠を順に見て、余裕があるものへ補給する。*/
			for (int iSlot = 0; iSlot < iSlotNum_; iSlot++)
			{
				if (!abHasWeapon_[iSlot])
					continue;

				/* 満タンの武器と、予備弾が無限のサブ武器は0が返るので数に入れない。*/
				if (aWeapons_[iSlot].AddReserveAmmo(iAmount) > 0)
					bAdded = true;
			}

			/* どれにも入らなければ、物資はその場に残してもらう。*/
			return bAdded;
		}


		bool WeaponInventory::RefillReserveAmmoToAll()
		{
			bool bAdded = false;

			/* 上限までのぶんをそのまま足せば満タンになる。満タンの武器と無限のサブ武器は0が返る。*/
			for (int iSlot = 0; iSlot < iSlotNum_; iSlot++)
			{
				if (!abHasWeapon_[iSlot])
					continue;

				if (aWeapons_[iSlot].AddReserveAmmo(aWeapons_[iSlot].GetMaxReserveAmmo()) > 0)
					bAdded = true;
			}

			return bAdded;
		}


		Weapon* WeaponInventory::GetCurrentWeapon()
		{
			return GetWeapon(enCurrentSlot_);
		}


		Weapon* WeaponInventory::GetWeapon(EnWeaponSlot enSlot)
		{
			/* どの枠でもない指定と、空の枠は持っていない扱い。*/
			if (!HasWeapon(enSlot))
				return nullptr;

			return &aWeapons_[static_cast<int>(enSlot)];
		}


		bool WeaponInventory::HasWeapon(EnWeaponSlot enSlot) const
		{
			/* どの枠でもない指定は持っていない扱い。*/
			if (enSlot == EnWeaponSlot::Num)
				return false;

			return abHasWeapon_[static_cast<int>(enSlot)];
		}


		EnWeaponSlot WeaponInventory::DecideSlot(EnWeaponType enNewType) const
		{
			/* その銃が本来入る枠。*/
			const EnWeaponSlot enNatural = nsData::WeaponStatusTable::Get(enNewType).enSlot_;
			const int iNatural = static_cast<int>(enNatural);

			/* 空いていれば素直にそこへ入れる。*/
			if (!abHasWeapon_[iNatural])
				return enNatural;

			/* 違う銃が入っているなら、いつも通り入れ替える。*/
			if (aWeapons_[iNatural].GetType() != enNewType)
				return enNatural;

			/*
			 * すでに同じ銃を持っている場合。
			 * サブ武器にかぎり、もう1丁をメイン枠へ回して二丁持ちにできる。
			 * メイン武器の二丁持ちは本家に無いので認めない。
			 */
			if (enNatural == EnWeaponSlot::Sub)
				return EnWeaponSlot::Main;

			/* 拾っても意味が無いので、その場に残す。*/
			return EnWeaponSlot::Num;
		}


		bool WeaponInventory::TryMergeDualWeapon()
		{
			const int iMain = static_cast<int>(EnWeaponSlot::Main);
			const int iSub = static_cast<int>(EnWeaponSlot::Sub);

			/* 両方の枠が埋まっていなければ揃っていない。*/
			if (!abHasWeapon_[iMain] || !abHasWeapon_[iSub])
				return false;

			/* 違う銃どうしはまとめられない。*/
			if (aWeapons_[iMain].GetType() != aWeapons_[iSub].GetType())
				return false;

			/* 2丁揃えても変化しない銃(マグナムなど)はそのまま2枠を占める。*/
			const EnWeaponType enDualType = aWeapons_[iSub].GetDualType();
			if (enDualType == EnWeaponType::Num)
				return false;

			/* サブ枠を二丁持ちへ変え、メイン枠を空ける。*/
			aWeapons_[iSub].Init(enDualType);
			abHasWeapon_[iMain] = false;
			EquipSlot(EnWeaponSlot::Sub);

			return true;
		}


		void WeaponInventory::EquipSlot(EnWeaponSlot enSlot)
		{
			/* 構える枠を変えて、構える動作を始める(リロード中だった場合は中断される)。*/
			enCurrentSlot_ = enSlot;
			aWeapons_[static_cast<int>(enSlot)].Deploy();
		}


		bool WeaponInventory::IsEmpty() const
		{
			/* どの枠にも入っていなければ未所持。*/
			for (int iSlot = 0; iSlot < iSlotNum_; iSlot++)
			{
				if (abHasWeapon_[iSlot])
					return false;
			}

			return true;
		}
	}
}
