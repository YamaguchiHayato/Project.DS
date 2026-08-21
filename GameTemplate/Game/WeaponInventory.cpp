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
		}


		void WeaponInventory::SwitchPrev()
		{
			/* 未所持なら何もしない。*/
			if (vecWeapons_.empty())
				return;

			/* 先頭なら末尾へ回り込む。*/
			iCurrentIndex_ = (iCurrentIndex_ - 1 + static_cast<int>(vecWeapons_.size())) % static_cast<int>(vecWeapons_.size());
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
