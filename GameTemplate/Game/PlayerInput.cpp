#include "stdafx.h"
#include "PlayerInput.h"
#include "system/system.h"

namespace nsApp
{
	namespace nsActor
	{
		void PlayerInput::Update()
		{
			/* 移動(WASD)。*/
			/* TODO: プロジェクト内に専用のKeyboardクラスがあれば、そちらのポーリングに */
			/*       差し替えてください(Mouseクラスと同じ設計思想で統一できます)。*/
			vMoveAxis_ = Vector3(0.0f, 0.0f, 0.0f);
			if (GetAsyncKeyState('W') & 0x8000) { vMoveAxis_.z += 1.0f; }
			if (GetAsyncKeyState('S') & 0x8000) { vMoveAxis_.z -= 1.0f; }
			if (GetAsyncKeyState('D') & 0x8000) { vMoveAxis_.x += 1.0f; }
			if (GetAsyncKeyState('A') & 0x8000) { vMoveAxis_.x -= 1.0f; }

			/* マウスカーソル位置(スプライト空間、中心原点)を照準に使う。*/
			/* g_mouse->BeginFrame()はエンジン側で毎フレーム呼ばれている前提。*/
			nsK2EngineLow::Mouse* pMouse = nsK2EngineLow::g_mouse;
			if (pMouse != nullptr) {
				vAimScreenPos_ = pMouse->GetSpritePos();

				/* 左クリックで射撃(押しっぱなし=フルオート連射、押した瞬間=単発)。連射間隔は武器側で制御。*/
				bFirePress_ = pMouse->IsPress(nsK2EngineLow::enMouseButtonLeft);
				bFireTrigger_ = pMouse->IsTrigger(nsK2EngineLow::enMouseButtonLeft);

				/* ホイールで武器切り替え(奥へ1ノッチ=次の武器、手前へ1ノッチ=前の武器)。*/
				const int iWheelDelta = pMouse->GetWheelDelta();
				bWeaponSwitchNextTrigger_ = (iWheelDelta > 0);
				bWeaponSwitchPrevTrigger_ = (iWheelDelta < 0);
			}
			else {
				/* g_mouseが未初期化の場合は何もしない(安全側に倒す)。*/
				bFirePress_ = false;
				bFireTrigger_ = false;
				bWeaponSwitchNextTrigger_ = false;
				bWeaponSwitchPrevTrigger_ = false;
			}

			/* カメラ旋回用のマウス横移動量を更新する(カーソルロック)。*/
			UpdateMouseLook();

			/* 各種トリガー入力(押した瞬間のみtrue)。*/
			bReloadTrigger_ = CheckTrigger('R', bPrevReloadPress_);
			bInteractTrigger_ = CheckTrigger('E', bPrevInteractPress_);
			bLightTrigger_ = CheckTrigger('F', bPrevLightPress_);
			bPauseTrigger_ = CheckTrigger(VK_ESCAPE, bPrevPausePress_);
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

			/* カーソルを中央へ戻す(端で止まらず無限に旋回できる)。*/
			SetCursorPos(center.x, center.y);
		}

		bool PlayerInput::CheckTrigger(int iVKey, bool& bPrevPress)
		{
			bool bIsPress = (GetAsyncKeyState(iVKey) & 0x8000) != 0;
			bool bIsTrigger = bIsPress && !bPrevPress;
			bPrevPress = bIsPress;
			return bIsTrigger;
		}
	}
}