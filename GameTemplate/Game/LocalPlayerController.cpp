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
			stOut.fLookPitchDelta_ = 0.0f;	/* TODO(Phase2): 縦のマウス移動量からピッチを作る。*/

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

			/* ライト・ポーズ。*/
			stOut.bLightTrigger_ = stInput_.IsLightTrigger();
			stOut.bPauseTrigger_ = stInput_.IsPauseTrigger();

			/*
			 * TODO(Phase1/2): shove(右クリック)・蘇生ホールド(Eキー長押し)を PlayerInput に
			 *   追加してここで充填する。今はまだ実装していないので false 固定。
			 */
			stOut.bShoveTrigger_ = false;
			stOut.bUseHold_ = false;
		}
	}
}
