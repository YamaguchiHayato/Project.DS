#include "stdafx.h"
#include "LocalPlayerController.h"

namespace nsApp
{
	namespace nsActor
	{
		void LocalPlayerController::PollIntent(PlayerIntent& stOut)
		{
			/* 実入力を1フレーム更新する。*/
			stInput_.Update();

			/* 移動入力(カメラ相対の軸)。*/
			stOut.vMoveAxis_ = stInput_.GetMoveAxis();

			/* 視点旋回: デバイスの横移動量(px)に感度を掛けてラジアン化し、Playerには角度で渡す。*/
			stOut.fLookYawDelta_ = stInput_.GetMouseDeltaX() * fLookSensitivity_;
			stOut.fLookPitchDelta_ = -stInput_.GetMouseDeltaY() * fLookPitchSensitivity_;	/* 上下反転なし: マウス上=視点上。縦は専用感度。*/

			/* 射撃。*/
			stOut.bFirePress_ = stInput_.IsFirePress();
			stOut.bFireTrigger_ = stInput_.IsFireTrigger();

			/* リロード。*/
			stOut.bReloadTrigger_ = stInput_.IsReloadTrigger();

			/* 使用・インタラクト(Eキー)。*/
			stOut.bUseTrigger_ = stInput_.IsInteractTrigger();

			/* 武器切替(ホイール)。*/
			stOut.bWeaponNextTrigger_ = stInput_.IsWeaponSwitchNextTrigger();
			stOut.bWeaponPrevTrigger_ = stInput_.IsWeaponSwitchPrevTrigger();

			/* 数字キーでの直接持ち替え(1=メイン, 2=サブ)。*/
			stOut.bMainWeaponTrigger_ = stInput_.IsMainWeaponTrigger();
			stOut.bSubWeaponTrigger_ = stInput_.IsSubWeaponTrigger();

			/* ライト・ポーズ。*/
			stOut.bLightTrigger_ = stInput_.IsLightTrigger();
			stOut.bPauseTrigger_ = stInput_.IsPauseTrigger();

			/* アイテム(回復/投擲)。*/
			stOut.bHealTrigger_ = stInput_.IsHealTrigger();
			stOut.bThrowTrigger_ = stInput_.IsThrowTrigger();

			/* スプリント(Shift)。*/
			stOut.bSprintPress_ = stInput_.IsSprintPress();

			/* 突き飛ばし(Vキー)。*/
			stOut.bShoveTrigger_ = stInput_.IsShoveTrigger();

			/* 覗き込み(右クリック)。*/
			stOut.bAdsPress_ = stInput_.IsAdsPress();

			/*
			 * TODO(Phase2): 蘇生ホールド(Eキー長押し)を PlayerInput に追加してここで充填する。
			 *   今はまだ実装していないので false 固定。
			 */
			stOut.bUseHold_ = false;
		}
	}
}
