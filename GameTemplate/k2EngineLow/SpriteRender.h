#pragma once
#pragma once

/// <summary>
/// スプライトクラス。
/// </summary>
namespace nsK2EngineLow {
	class SpriteRender:public IRender
	{
	public:
		/// <summary>
		/// 画像の初期化。
		/// </summary>
		/// <param name="filePath">画像のファイルパス</param>
		/// <param name="w">画像の横幅</param>
		/// <param name="h">画像の縦幅</param>
		void Init(const char* filePath, const float w, const float h, AlphaBlendMode alphaBlendMode = AlphaBlendMode_Trans);
		/// <summary>
		/// 半円に沿って画像を切り取る時用のの初期化。
		/// </summary>
		void Init(const char* filePath, const float w, const float h, const char* fxPath, AlphaBlendMode alphaBlendMode = AlphaBlendMode_Trans);

		// ==========================================
		// ★ここを追加：指定したピクセル範囲だけを切り取る
		// ==========================================
		void SetSourceRect(float x, float y, float w, float h)
		{
			m_sprite.SetSourceRect(x, y, w, h);
		}

		/// <summary>
		/// 座標の設定。
		/// </summary>
		/// <param name="pos">座標</param>
		void SetPosition(const Vector3& pos)
		{
			m_position = pos;
		}
		/// <summary>
		/// 座標の取得。
		/// </summary>
		/// <returns>座標</returns>
		const Vector3& GetPosition() const
		{
			return m_position;
		}
		/// <summary>
		/// 大きさの設定。
		/// </summary>
		/// <param name="scale">大きさ</param>
		void SetScale(const Vector3& scale)
		{
			m_scale = scale;
		}
		/// <summary>
		/// 大きさの取得。
		/// </summary>
		/// <returns>大きさ</returns>
		const Vector3& GetScale() const
		{
			return m_scale;
		}
		

		void SetMulColor(const Vector4& mulColor)

		{

			m_sprite.SetMulColor(mulColor);

		}

		void SetRotation(const Quaternion& rot)
		{
			m_rotation = rot;
		}
	
		const Quaternion& GetRotation() const
		{
			return m_rotation;
		}
		/// <summary>
		/// 中心点を設定。
		/// </summary>
		/// <param name="pivot">中心点</param>
		void SetPivot(const Vector2& pivot)
		{
			m_pivot = pivot;
		}
		/// <summary>
		/// 中心点を取得。
		/// </summary>
		/// <returns>中心点</returns>
		const Vector2& GetPivot() const
		{
			return m_pivot;
		}
		/// <summary>
		/// 更新。
		/// </summary>
		void Update()
		{
			if (m_isActive) {
				m_sprite.Update(m_position,
					m_rotation,
					m_scale,
					m_pivot);
			}
		}
		/// <summary>
		/// 描画
		/// </summary>
		/// <param name="rc">RenderContext</param>
		void Draw(RenderContext& rc);
		
		void SetActive(bool isActive)
		{
			m_isActive = isActive;
		}

		bool GetActive()
		{
			return m_isActive;
		}


	private:
		void OnRender2D(RenderContext& rc) override
		{
			m_sprite.Draw(rc);
		}
		bool			m_isActive = true;
		Sprite			m_sprite;								//スプライトクラスの参照
		Vector3			m_position=Vector3::Zero;				//座標
		Quaternion		m_rotation = Quaternion::Identity;		//向き
		Vector3			m_scale = Vector3::One;					//大きさ
		Vector2			m_pivot = Sprite::DEFAULT_PIVOT;		//画像の中点

	};
}

