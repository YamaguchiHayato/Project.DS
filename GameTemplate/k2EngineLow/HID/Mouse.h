#pragma once

namespace nsK2EngineLow {

	class SpriteRender;

	/// <summary>
	/// マウスボタン定義
	/// </summary>
	enum EnMouseButton {
		enMouseButtonLeft,		//!<左ボタン。
		enMouseButtonRight,		//!<右ボタン。
		enMouseButtonMiddle,	//!<中ボタン（ホイール押し込み）。
		enMouseButtonNum,		//!<ボタンの数。
	};

	/// <summary>
	/// マウスクラス。
	/// GamePadと同じく、エンジン(K2EngineLow)が毎フレーム BeginFrame() を呼んで状態を更新する。
	/// 使う側は g_mouse から位置・ボタン・ホイールを取得するだけでよい。
	///
	/// 【座標について】
	///   2D座標空間はスプライト空間（±FRAME_BUFFER/2）とフォント/UI空間（±UI_SPACE/2）の
	///   2種類があるため、両方のゲッターを用意している。スプライトとの当たり判定には
	///   必ず GetSpritePos() / IsOver() を使うこと。
	///
	/// 【ホイールについて】
	///   ホイールはポーリングで取得できないため、アプリケーションのウィンドウプロシージャから
	///   Mouse::ProcessMessage(msg, wParam) を呼んで転送してもらう（1行）。
	///   呼ばなくてもホイール以外の機能は動作する。
	/// </summary>
	class Mouse : public Noncopyable {
	public:
		/// <summary>
		/// 初期化。K2EngineLow::Init から呼ばれる。
		/// </summary>
		/// <param name="hwnd">ゲームウィンドウのハンドル</param>
		void Init(HWND hwnd) { m_hwnd = hwnd; }

		/// <summary>
		/// フレーム先頭の状態更新。K2EngineLow::BeginFrame から呼ばれる。
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// ウィンドウメッセージの転送口（ホイール用）。
		/// アプリのWndProcのWM_MOUSEWHEELからそのまま呼ぶこと。
		/// </summary>
		static void ProcessMessage(UINT msg, WPARAM wParam);

		/// <summary>カーソル位置（スプライト空間。中心原点 ±FRAME_BUFFER/2、Y上向き）</summary>
		const Vector2& GetSpritePos() const { return m_spritePos; }
		/// <summary>カーソル位置（フォント/UI空間。中心原点 ±UI_SPACE/2、Y上向き）</summary>
		const Vector2& GetUIPos() const { return m_uiPos; }

		/// <summary>押した瞬間か</summary>
		bool IsTrigger(EnMouseButton button) const { return m_now[button] && !m_prev[button]; }
		/// <summary>押されているか</summary>
		bool IsPress(EnMouseButton button) const { return m_now[button]; }
		/// <summary>離した瞬間か</summary>
		bool IsRelease(EnMouseButton button) const { return !m_now[button] && m_prev[button]; }

		/// <summary>
		/// このフレームのホイール回転量（ノッチ単位。+1=奥へ1ノッチ / -1=手前へ1ノッチ）。
		/// WndProcからProcessMessageが呼ばれていない場合は常に0。
		/// </summary>
		int GetWheelDelta() const { return m_wheelDelta; }

		/// <summary>
		/// スプライト空間の矩形（中心center、幅w、高さh）にカーソルが乗っているか。
		/// w/h には「Init時のサイズ×スケール」を渡すこと。
		/// </summary>
		bool IsOver(const Vector3& center, float w, float h) const
		{
			return fabsf(m_spritePos.x - center.x) <= w * 0.5f
				&& fabsf(m_spritePos.y - center.y) <= h * 0.5f;
		}
		/// <summary>スプライトにカーソルが乗っているか（中心ピボット前提）</summary>
		bool IsOver(const SpriteRender& sprite, float w, float h) const;

	private:
		HWND m_hwnd = nullptr;
		Vector2 m_spritePos;					// スプライト空間でのカーソル位置
		Vector2 m_uiPos;						// フォント/UI空間でのカーソル位置
		bool m_now[enMouseButtonNum] = {};		// 今フレームのボタン状態
		bool m_prev[enMouseButtonNum] = {};		// 前フレームのボタン状態
		int m_wheelDelta = 0;					// 今フレームのホイール回転量（ノッチ）
		static int s_wheelAccum;				// WndProcから蓄積される生のホイール値
	};

	extern Mouse* g_mouse;	// マウス。
}
