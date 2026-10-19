#include "stdafx.h"
#include "PlayerCalc.h"

namespace nsApp
{
	namespace nsActor
	{
		void PlayerCalc::Init()
		{
			/* TODO: 計算に必要な参照の結び付け。*/
		}


		float PlayerCalc::GetCurrentSpread() const
		{
			/* TODO: Player::GetCurrentSpread をコメントごと移す。*/
			return 0.0f;
		}


		Vector3 PlayerCalc::MakeSpreadDirection(const Vector3& vAimDir) const
		{
			/* TODO: Player::MakeSpreadDirection をコメントごと移す。*/
			return vAimDir;
		}


		void PlayerCalc::CalcReloadMotion(float fReloadRate, float& fOutLower, float& fOutInsert, float& fOutSettle) const
		{
			/* TODO: Player::CalcReloadMotion をコメントごと移す。*/
			(void)fReloadRate;
			fOutLower = 0.0f;
			fOutInsert = 0.0f;
			fOutSettle = 0.0f;
		}


		void PlayerCalc::ApplyFireRecoil(nsWeapon::Weapon* pWeapon)
		{
			/* TODO: Player::ApplyFireRecoil をコメントごと移す。*/
			(void)pWeapon;
		}


		void PlayerCalc::FindHitEnemies(const Vector3& vRayStart, const Vector3& vRayDirection, int iMaxCount, std::vector<struct HitEnemy>& vecOutHits)
		{
			/* TODO: Player::FindHitEnemies をコメントごと移す。*/
			(void)vRayStart;
			(void)vRayDirection;
			(void)iMaxCount;
			(void)vecOutHits;
		}


		void PlayerCalc::FireHitScan(nsWeapon::Weapon* pWeapon, const Vector3& vEyePos, const Vector3& vMuzzlePos, const Vector3& vAimDir)
		{
			/* TODO: Player::FireHitScan をコメントごと移す。*/
			(void)pWeapon;
			(void)vEyePos;
			(void)vMuzzlePos;
			(void)vAimDir;
		}
	}
}