#include "stdafx.h"
#include "InGameHud.h"
#include "Player.h"
#include "Weapon.h"
#include <cstdio>

namespace
{
	const Vector3	vCrosshairPos_ = { -8.0f, 12.0f, 0.0f };	//! 中央クロスヘア。
	const Vector3	vHpPos_ = { -900.0f, -470.0f, 0.0f };		//! HP(左下)。
	const Vector3	vAmmoPos_ = { 700.0f, -470.0f, 0.0f };		//! 弾数(右下)。
	const Vector3	vObjectivePos_ = { -420.0f, 500.0f, 0.0f };	//! 目標(上)。
	const Vector3	vStatusPos_ = { -230.0f, 140.0f, 0.0f };	//! 状態(中央やや上)。
	const float		fHudFontScale_ = 0.8f;						//! 通常表示の大きさ。
	const float		fStatusFontScale_ = 2.0f;					//! 状態表示の大きさ。
}

namespace nsApp
{
	namespace nsUI
	{
		bool InGameHud::Start()
		{
			/* クロスヘア(中央の+)。*/
			stCrosshair_.SetPosition(vCrosshairPos_);
			stCrosshair_.SetScale(1.0f);
			stCrosshair_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stCrosshair_.SetText(L"+");

			/* HP(左下・緑寄り)。*/
			stHpText_.SetPosition(vHpPos_);
			stHpText_.SetScale(fHudFontScale_);
			stHpText_.SetColor(0.6f, 1.0f, 0.6f, 1.0f);
			stHpText_.SetText(wcHp_);

			/* 弾数(右下)。*/
			stAmmoText_.SetPosition(vAmmoPos_);
			stAmmoText_.SetScale(fHudFontScale_);
			stAmmoText_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stAmmoText_.SetText(wcAmmo_);

			/* 目標(上・固定文言)。*/
			stObjective_.SetPosition(vObjectivePos_);
			stObjective_.SetScale(fHudFontScale_);
			stObjective_.SetColor(1.0f, 0.9f, 0.4f, 1.0f);
			stObjective_.SetText(L"OBJECTIVE: REACH THE SAFE ROOM");

			/* 状態(ダウン等。通常時は空・赤)。*/
			stStatusText_.SetPosition(vStatusPos_);
			stStatusText_.SetScale(fStatusFontScale_);
			stStatusText_.SetColor(1.0f, 0.3f, 0.3f, 1.0f);
			stStatusText_.SetText(wcStatus_);
			return true;
		}


		void InGameHud::Update()
		{
			/* プレイヤーを取得する。いなければ表示を空にする。*/
			nsActor::Player* pPlayer = FindGO<nsActor::Player>("player");
			if (pPlayer == nullptr)
			{
				wcHp_[0] = L'\0';
				wcAmmo_[0] = L'\0';
				wcStatus_[0] = L'\0';
				stHpText_.SetText(wcHp_);
				stAmmoText_.SetText(wcAmmo_);
				stStatusText_.SetText(wcStatus_);
				return;
			}

			/* HP。*/
			swprintf_s(wcHp_, L"HP %d/%d", pPlayer->GetCurrentHP(), pPlayer->GetMaxHP());
			stHpText_.SetText(wcHp_);

			/* 弾数(リロード中は表示を切り替える)。武器名(narrow)は %hs で埋め込む。*/
			nsWeapon::Weapon* pWeapon = pPlayer->GetEquippedWeapon();
			if (pWeapon != nullptr)
			{
				if (pWeapon->IsReloading())
					swprintf_s(wcAmmo_, L"%hs  RELOADING", pWeapon->GetName());
				else
					swprintf_s(wcAmmo_, L"%hs  %d/%d", pWeapon->GetName(), pWeapon->GetCurrentAmmo(), pWeapon->GetMaxAmmo());
			}
			else
			{
				wcAmmo_[0] = L'\0';
			}
			stAmmoText_.SetText(wcAmmo_);

			/* 状態(ダウン中は出血残り秒を大きく表示。それ以外は空)。*/
			if (pPlayer->GetLifeState() == nsActor::EnLifeState::Down)
			{
				const int iRemain = static_cast<int>(pPlayer->GetBleedOutRemain()) + 1;
				swprintf_s(wcStatus_, L"DOWNED  -  %d", iRemain);
			}
			else
			{
				wcStatus_[0] = L'\0';
			}
			stStatusText_.SetText(wcStatus_);
		}


		void InGameHud::Render(RenderContext& rc)
		{
			/* クロスヘアと各種テキストを描画する。*/
			stCrosshair_.Draw(rc);
			stHpText_.Draw(rc);
			stAmmoText_.Draw(rc);
			stObjective_.Draw(rc);
			stStatusText_.Draw(rc);
		}
	}
}
