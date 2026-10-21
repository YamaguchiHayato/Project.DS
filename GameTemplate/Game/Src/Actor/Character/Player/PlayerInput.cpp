#include "stdafx.h"
#include "PlayerInput.h"
#include "system/system.h"
#include "Src/InputObserver.h"

namespace nsApp
{
	namespace nsActor
	{
		void PlayerInput::Update()
		{
			nsData::InputObserver& input_ = nsData::InputObserver::Get();

			/* 移動。割り当ては InputObserver が持つ。*/
			vMoveAxis_ = Vector3(0.0f, 0.0f, 0.0f);
			if (input_.IsPress(nsData::EnAction::MoveForward)) { vMoveAxis_.z += 1.0f; }
			if (input_.IsPress(nsData::EnAction::MoveBack)) { vMoveAxis_.z -= 1.0f; }
			if (input_.IsPress(nsData::EnAction::MoveRight)) { vMoveAxis_.x += 1.0f; }
			if (input_.IsPress(nsData::EnAction::MoveLeft)) { vMoveAxis_.x -= 1.0f; }

			/* 照準の位置だけ、マウスカーソルから取る。*/
			nsK2EngineLow::Mouse* pMouse = nsK2EngineLow::g_mouse;
			if (pMouse != nullptr)
				vAimScreenPos_ = pMouse->GetSpritePos();

			bFirePress_ = input_.IsPress(nsData::EnAction::Fire);
			bFireTrigger_ = input_.IsTrigger(nsData::EnAction::Fire);
			bAdsPress_ = input_.IsPress(nsData::EnAction::Ads);
			bWeaponSwitchNextTrigger_ = input_.IsTrigger(nsData::EnAction::WeaponNext);
			bWeaponSwitchPrevTrigger_ = input_.IsTrigger(nsData::EnAction::WeaponPrev);

			/* カメラ旋回用のマウス移動量。*/
			UpdateMouseLook();

			bReloadTrigger_ = input_.IsTrigger(nsData::EnAction::Reload);
			bInteractTrigger_ = input_.IsTrigger(nsData::EnAction::Interact);
			bLightTrigger_ = input_.IsTrigger(nsData::EnAction::Light);
			bPauseTrigger_ = input_.IsTrigger(nsData::EnAction::Pause);
			bHealPress_ = input_.IsPress(nsData::EnAction::Heal);
			bThrowTrigger_ = input_.IsTrigger(nsData::EnAction::Throw);
			bQuickItemTrigger_ = input_.IsTrigger(nsData::EnAction::QuickItem);
			bSprintPress_ = input_.IsPress(nsData::EnAction::Sprint);
			bCrouchPress_ = input_.IsPress(nsData::EnAction::Crouch);
			bShoveTrigger_ = input_.IsTrigger(nsData::EnAction::Shove);
			bMainWeaponTrigger_ = input_.IsTrigger(nsData::EnAction::MainWeapon);
			bSubWeaponTrigger_ = input_.IsTrigger(nsData::EnAction::SubWeapon);
		}

		PlayerInput::~PlayerInput()
		{
			/* カーソルを非表示にしていたら表示に戻す。*/
			if (bCursorLocked_)
				ShowCursor(TRUE);
		}

		void PlayerInput::UpdateMouseLook()
		{
			fMouseDeltaX_ = 0.0f;
			fMouseDeltaY_ = 0.0f;

			/* ウィンドウがフォアグラウンドの時だけカーソルをロックする。*/
			if (g_hWnd == nullptr || GetForegroundWindow() != g_hWnd)
				return;

			/* 初回にカーソルを非表示にする。*/
			if (!bCursorLocked_)
			{
				ShowCursor(FALSE);
				bCursorLocked_ = true;
			}

			/* クライアント領域の中央のスクリーン座標を求める。*/
			RECT rc;
			GetClientRect(g_hWnd, &rc);
			POINT center = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
			ClientToScreen(g_hWnd, &center);

			/* 現在のカーソル位置と中央との横方向の差分を取る。*/
			POINT cursor;
			GetCursorPos(&cursor);
			fMouseDeltaX_ = static_cast<float>(cursor.x - center.x);
			fMouseDeltaY_ = static_cast<float>(cursor.y - center.y);

			/* カーソルを中央へ戻す(端で止まらず無限に旋回できる)。*/
			SetCursorPos(center.x, center.y);
		}

		bool PlayerInput::IsKeyPress(int iVKey) const
		{
			return (GetAsyncKeyState(iVKey) & 0x8000) != 0;
		}

		bool PlayerInput::CheckTrigger(int iVKey, bool& bPrevPress)
		{
			return CheckTriggerFromPress(IsKeyPress(iVKey), bPrevPress);
		}

		bool PlayerInput::CheckTriggerFromPress(bool bIsPress, bool& bPrevPress)
		{
			/* 前回押されておらず、今回押されていれば「押した瞬間」。*/
			const bool bIsTrigger = bIsPress && !bPrevPress;
			bPrevPress = bIsPress;
			return bIsTrigger;
		}
	}
}