/*!
*@brief	マウス。
*/
#include "k2EngineLowPreCompile.h"
#include "Mouse.h"
#include "../SpriteRender.h"

namespace nsK2EngineLow {

	Mouse* g_mouse = nullptr;
	int Mouse::s_wheelAccum = 0;

	void Mouse::ProcessMessage(UINT msg, WPARAM wParam)
	{
		if (msg == WM_MOUSEWHEEL) {
			// メッセージポンプはメインスレッドなので排他は不要
			s_wheelAccum += GET_WHEEL_DELTA_WPARAM(wParam);
		}
	}

	void Mouse::BeginFrame()
	{
		// ==========================================
		// ホイール：蓄積値をノッチ単位に変換（端数は次フレームへ持ち越す。
		// 高解像度ホイールでもノッチ単位で安定して取れる）
		// ==========================================
		m_wheelDelta = s_wheelAccum / WHEEL_DELTA;
		s_wheelAccum -= m_wheelDelta * WHEEL_DELTA;

		// ==========================================
		// カーソル位置：クライアント座標 → 各2D空間へ変換
		// ウィンドウサイズは可変なので毎フレーム実サイズで正規化する
		// ==========================================
		POINT p = { 0, 0 };
		GetCursorPos(&p);
		RECT rc = { 0, 0, (LONG)FRAME_BUFFER_W, (LONG)FRAME_BUFFER_H };
		if (m_hwnd) {
			ScreenToClient(m_hwnd, &p);
			GetClientRect(m_hwnd, &rc);
		}
		float clientW = static_cast<float>(rc.right - rc.left);
		float clientH = static_cast<float>(rc.bottom - rc.top);
		if (clientW < 1.0f) clientW = 1.0f;
		if (clientH < 1.0f) clientH = 1.0f;

		const float nx = static_cast<float>(p.x) / clientW;	// 0.0(左)〜1.0(右)
		const float ny = static_cast<float>(p.y) / clientH;	// 0.0(上)〜1.0(下)

		// 中心原点・Y上向きへ変換（スクリーンはY下向きなので反転）
		m_spritePos.x = nx * FRAME_BUFFER_W - FRAME_BUFFER_W * 0.5f;
		m_spritePos.y = FRAME_BUFFER_H * 0.5f - ny * FRAME_BUFFER_H;
		m_uiPos.x = nx * UI_SPACE_WIDTH - UI_SPACE_WIDTH * 0.5f;
		m_uiPos.y = UI_SPACE_HEIGHT * 0.5f - ny * UI_SPACE_HEIGHT;

		// ==========================================
		// ボタン：GamePadと同じGetAsyncKeyStateポーリング。
		// 他のウィンドウ操作中に反応しないよう、フォアグラウンド時のみ有効にする
		// ==========================================
		const bool focused = (m_hwnd == nullptr) || (GetForegroundWindow() == m_hwnd);
		static const int vkTable[enMouseButtonNum] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
		for (int i = 0; i < enMouseButtonNum; i++) {
			m_prev[i] = m_now[i];
			m_now[i] = focused && (GetAsyncKeyState(vkTable[i]) & 0x8000) != 0;
		}
	}

	bool Mouse::IsOver(const SpriteRender& sprite, float w, float h) const
	{
		return IsOver(sprite.GetPosition(), w, h);
	}
}
