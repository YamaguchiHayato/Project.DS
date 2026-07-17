#pragma once
namespace nsK2EngineLow {
	class FontRender:public IRender
	{
	public:
		static const int MAX_TEXT_SIZE = 256;
		~FontRender()
		{
		}
		//テキストの設定。
		void SetText(const wchar_t* text)
		{
			// ★以前は swprintf_s(m_text, text) だったため、text を「書式文字列」として
			//   解釈してしまい、'%' を含む文字列（例:"100%"）で不正パラメータ→即死していた。
			//   ここは書式化ではなく単純コピーが正しい。長すぎる場合は安全に切り詰める。
			if (text == nullptr) { m_text[0] = L'\0'; return; }
			wcsncpy_s(m_text, MAX_TEXT_SIZE, text, _TRUNCATE);
		}
		//テキストを取得。
		const wchar_t* GetText() const
		{
			return m_text;
		}
		//ポジションの設定。
		void SetPosition(float x, float y, float z)
		{
			SetPosition({ x, y, z });
		}
	
		void SetPosition(const Vector3& position)
		{
			m_position = position;
		}
		// ポジションの取得。
		const Vector3& GetPosition() const
		{
			return m_position;
		}
		// スケールの設定。
		void SetScale(const float scale)
		{
			m_scale = scale;
		}
		// スケールの取得。
		const float GetScale() const
		{
			return m_scale;
		}
		// 色の設定。
		void SetColor(float r, float g, float b, float a)
		{
			SetColor({ r, g, b, a });
		}
		void SetColor(const Vector4& color)
		{
			m_color = color;
		}
		// 色の取得。
		const Vector4& GetColor() const
		{
			return m_color;
		}
		// 回転の設定。
		void SetRotation(const float rotation)
		{
			m_rotation = rotation;
		}
		// 回転の取得。
		const float GetRotation() const
		{
			return m_rotation;
		}
		// ピボットの設定。
		void SetPivot(float x, float y)
		{
			SetPivot({ x, y });
		}
		void SetPivot(const Vector2& pivot)
		{
			m_pivot = pivot;
		}
		// ピボットの取得。
		const Vector2& GetPivot() const
		{
			return m_pivot;
		}
		//影を描画するかどうかの設定。
		void SetisDrawShadow(bool isDrawShadow)
		{
			m_font.SetIsDrawShadow(isDrawShadow);
		}
		//描画。
		void Draw(RenderContext& rc);

		// 現在のテキストを現在のスケールで描画したときのピクセルサイズ(幅,高さ)を返す。
		// UIの中央寄せ配置に使う。
		Vector2 MeasureText() const
		{
			return m_font.MeasureString(m_text, m_scale);
		}

		// 親スプライトを設定する。設定すると、このフォントは毎フレーム
		// 「親スプライトの中心」に中央寄せで追従して描画される。
		// スプライトはFRAME_BUFFER基準、フォントはUI_SPACE基準で座標空間が異なるが、
		// その違いも内部で吸収するので、プレートに乗せたいラベルはこれを呼ぶだけでよい。
		// この場合 SetPosition は「親中心からのローカルオフセット(UI_SPACE基準ピクセル)」として扱う。
		void SetParentSprite(const SpriteRender* parent) { m_parent = parent; }
		void ClearParentSprite() { m_parent = nullptr; }
		const SpriteRender* GetParentSprite() const { return m_parent; }

		
		void SetShadowParam(bool isDrawShadow, float shadowOffset, const Vector4& shadowColor)
		{
			m_font.SetShadowParam(isDrawShadow, shadowOffset, shadowColor);
		}

	private:
		void OnRender2D(RenderContext& rc) override
		{
			if (m_text == nullptr)
			{
				return;
			}

			// 色にアルファ値をあらかじめ掛ける（既存挙動を維持）
			m_color.r *= m_color.a;
			m_color.g *= m_color.a;
			m_color.b *= m_color.a;

			Vector2 drawPos(m_position.x, m_position.y);
			Vector2 drawPivot = m_pivot;

			if (m_parent != nullptr) {
				// 親スプライトの中心をフォント空間へ変換する。
				//   スプライト: 正射影が実バックバッファ(FRAME_BUFFER)基準 → 画面端が ±FRAME_BUFFER/2
				//   フォント  : UI_SPACE基準 → 画面端が ±UI_SPACE/2
				// 同じ画面位置に来るよう、親座標に (UI_SPACE / FRAME_BUFFER) を掛けて変換する。
				const Vector3& pp = m_parent->GetPosition();
				const float ratioX = static_cast<float>(UI_SPACE_WIDTH) / static_cast<float>(FRAME_BUFFER_W);
				const float ratioY = static_cast<float>(UI_SPACE_HEIGHT) / static_cast<float>(FRAME_BUFFER_H);
				float cx = pp.x * ratioX + m_position.x; // m_position は親中心からのローカルオフセット
				float cy = pp.y * ratioY + m_position.y;
				// テキスト実寸を測り、中心が (cx,cy) になる左上座標を求める。
				// （フォントは左上基準で下方向へ伸びるので、上端 = 中心 + 高さ/2）
				Vector2 sz = m_font.MeasureString(m_text, m_scale);
				drawPos.x = cx - sz.x * 0.5f;
				drawPos.y = cy + sz.y * 0.5f;
				drawPivot = Vector2(0.0f, 1.0f); // Font::Drawの (1-y) 反転で origin(0,0)=左上 になる
			}

			m_font.Begin(rc);
			m_font.Draw(m_text, drawPos, m_color, m_rotation, m_scale, drawPivot);
			m_font.End(rc);
		}
		Vector3							m_position = Vector3::Zero;				
		float							m_scale = 1.0f;						
		Vector4							m_color = g_vec4White;				
		float							m_rotation = 0.0f;					
		Vector2							m_pivot = Sprite::DEFAULT_PIVOT;
		const SpriteRender*				m_parent = nullptr;	// 追従先の親スプライト（nullなら通常描画）
		wchar_t							m_text[MAX_TEXT_SIZE];
		Font							m_font;
	};
}
