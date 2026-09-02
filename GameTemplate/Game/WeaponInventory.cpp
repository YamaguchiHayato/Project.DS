#include "stdafx.h"
#include "WeaponInventory.h"

namespace nsApp
{
	namespace nsWeapon
	{
		void WeaponInventory::AddWeapon(EnWeaponType enType)
		{
			/* 新しい武器を末尾に追加し、種類で初期化する。*/
			Weapon weapon;
			weapon.Init(enType);
			vecWeapons_.push_back(weapon);
		}


		void WeaponInventory::Update(float fDeltaTime)
		{
			/* 現在の武器だけ更新する。*/
			Weapon* pCurrent = GetCurrentWeapon();
			if (pCurrent != nullptr)
				pCurrent->Update(fDeltaTime);
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


		void WeaponInventory::SwitchNext()
		{
			/* 未所持なら何もしない。*/
			if (vecWeapons_.empty())
				return;

			/* 末尾なら先頭へ回り込む。*/
			iCurrentIndex_ = (iCurrentIndex_ + 1) % static_cast<int>(vecWeapons_.size());

			/* 持ち替えたので構える動作を始める(リロード中だった場合は中断される)。*/
			vecWeapons_[iCurrentIndex_].Deploy();
		}


		void WeaponInventory::SwitchPrev()
		{
			/* 未所持なら何もしない。*/
			if (vecWeapons_.empty())
				return;

			/* 先頭なら末尾へ回り込む。*/
			iCurrentIndex_ = (iCurrentIndex_ - 1 + static_cast<int>(vecWeapons_.size())) % static_cast<int>(vecWeapons_.size());

			/* 持ち替えたので構える動作を始める(リロード中だった場合は中断される)。*/
			vecWeapons_[iCurrentIndex_].Deploy();
		}


		void WeaponInventory::SwitchToSlot(EnWeaponSlot enSlot)
		{
			/* 所持リストから、指定された区分の武器を探す。*/
			for (int i = 0; i < static_cast<int>(vecWeapons_.size()); i++)
			{
				/* 区分が違えば次へ。*/
				if (vecWeapons_[i].GetSlot() != enSlot)
					continue;

				/* すでに持っている武器なら持ち替えない(構え直しを起こさない)。*/
				if (i == iCurrentIndex_)
					return;

				/* 見つかったので持ち替える。*/
				iCurrentIndex_ = i;
				vecWeapons_[iCurrentIndex_].Deploy();
				return;
			}
		}


		bool WeaponInventory::AddReserveAmmoToAll(int iAmount)
		{
			bool bAdded = false;

			/* 所持している武器を順に見て、余裕があるものへ補給する。*/
			for (Weapon& weapon : vecWeapons_)
			{
				if (weapon.AddReserveAmmo(iAmount) > 0)
					bAdded = true;
			}

			return bAdded;
		}


		Weapon* WeaponInventory::GetCurrentWeapon()
		{
			/* 未所持ならnullptrを返す。*/
			if (vecWeapons_.empty())
				return nullptr;

			return &vecWeapons_[iCurrentIndex_];
		}
	}
}
