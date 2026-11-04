#include "stdafx.h"
#include "LocalPlayerController.h"
#include "GraphicsSetting.h"

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

			/* 視点旋回。設定の感度と上下左右の反転を掛ける。 */
			nsData::GraphicsSetting& setting = nsData::GraphicsSetting::Get();
			const float sensitivity = setting.GetSensitivity();
			const float invertX = setting.GetCamera().IsON(nsData::CameraItem::InvertX) ? -1.0f : 1.0f;
			const float invertY = setting.GetCamera().IsON(nsData::CameraItem::InvertY) ? -1.0f : 1.0f;
			stOut.fLookYawDelta_ = stInput_.GetMouseDeltaX() * fLookSensitivity_ * sensitivity * invertX;
			stOut.fLookPitchDelta_ = -stInput_.GetMouseDeltaY() * fLookPitchSensitivity_ * sensitivity * invertY;

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

			/* アイテム(メディキット/投擲/即効アイテム)。*/
			stOut.bHealPress_ = stInput_.IsHealPress();
			stOut.bThrowTrigger_ = stInput_.IsThrowTrigger();
			stOut.bQuickItemTrigger_ = stInput_.IsQuickItemTrigger();

			/* スプリント(Shift)としゃがみ(Ctrl)。*/
			stOut.bSprintPress_ = stInput_.IsSprintPress();
			stOut.bCrouchPress_ = stInput_.IsCrouchPress();

			/* 突き飛ばし(Vキー)。*/
			stOut.bShoveTrigger_ = stInput_.IsShoveTrigger();

			/* 覗き込み(右クリック)。*/
			/* 覗き込み。設定が切り替えなら、押した瞬間にオンオフする。 */
			if (nsData::GraphicsSetting::Get().GetControl().IsON(nsData::ControlItem::AdsToggle))
			{
				/* 右クリックを押した瞬間に、構え状態を切り替える。 */
				if (stInput_.IsAdsTrigger())
					bAdsToggleOn_ = !bAdsToggleOn_;

				/* 現在の構え状態を Intent に反映する。 */
				stOut.bAdsPress_ = bAdsToggleOn_;
			}
			else
			{
				bAdsToggleOn_ = false;
				stOut.bAdsPress_ = stInput_.IsAdsPress();
			}

			/*
			 * TODO(Phase2): 蘇生ホールド(Eキー長押し)を PlayerInput に追加してここで充填する。
			 *   今はまだ実装していないので false 固定。
			 */
			stOut.bUseHold_ = false;
		}
	}
}
