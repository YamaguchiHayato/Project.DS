#pragma once

//スカイキューブクラス。
//SetType関数を呼んで使いたい背景に設定してね。
namespace nsK2EngineLow {
	enum EnSkyCubeType {
		enSkyCubeType_Day,		// 昼間
		enSkyCubeType_Night,	// 夜間
		enSkyCubeType_Snow,		// 雪山
		enSkyCubeType_Snow_2,	// 雪山_2
		enSkyCubeType_Wild,		// 荒野
		enSkyCubeType_Wild_2,	// 荒野２
		enSkyCubeType_Wild_Night,	// 荒野(夜間)
		enSkyCubeType_Grass,	// 芝生
		enSkyCubeType_Euro,		// 欧州
		enSkyCubeType_DayToon,	// 昼間(トゥーン調)
		enSkyCubeType_DayToon_2,// 昼間(トゥーン調)２
		enSkyCubeType_DayToon_3,// 昼間(トゥーン調)３
		enSkyCubeType_DayToon_4,// 昼間(トゥーン調)４
		enSkyCubeType_NightToon, // 夜間(トゥーン調)
		enSkyCubeType_NightToon_2, // 夜間(トゥーン調)２
		enSkyCubeType_SunriseToon,	// 明け方(トゥーン調)
		//enSkyCubeType_SpaceToon,	// 大気圏(トゥーン調)
		enSkyCubeType_SpaceToon_2,	// 大気圏(トゥーン調)２
		enSkyCubeType_Num,
	};
	class SkyCube:public IGameObject
	{
	public:
		SkyCube();
		~SkyCube();
		bool Start();
		void Update();
		void Render(RenderContext& rc);

		void SetPosition(const Vector3& pos) 
		{
			m_position = pos;
			m_isDirty = true;
		}

		void SetScale(const Vector3& scal)
		{
			m_scale = scal;
			m_isDirty = true;
		}
		void SetScale(const float scale)
		{
			m_scale = g_vec3One;
			m_scale.Scale(scale);
			m_isDirty = true;
		}
		//どのスカイキューブのテクスチャを使うか。
		void SetType(EnSkyCubeType type)
		{
			m_type = type;
		}
		//明るさを設定。
		void SetLuminance(float lum)
		{
			m_luminance = lum;
		}
		//現在使用中のファイルパスの確認用。
		const wchar_t* GetTextureFilePath()
		{
			return m_textureFilePaths[m_type];
		}

	private:
		ModelRender m_modelRender;

		Texture m_texture[enSkyCubeType_Num];
		const wchar_t* m_textureFilePaths[enSkyCubeType_Num];
		Vector3 m_position = g_vec3Zero;
		Vector3 m_scale = g_vec3One * 50.0f;
		float m_luminance = 1.0f;
		bool m_isDirty = false;//変更されたらtrueになる。Dirtyの意味はDirty Copyってもので使い古されたものだけどまだ使えるものらしい。
		EnSkyCubeType m_type = enSkyCubeType_Night;
		Quaternion m_rot = { 0.0f,0.0f,0.0f,1.0f };
	};
}

