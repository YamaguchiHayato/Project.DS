#pragma once
namespace nsK2EngineLow {
	struct DirectionLight {
		Vector3 m_direction;//ライトの方向。3要素のベクトルで表現。
		float pad0=0.0f;
		Vector3 m_color;//ライトのカラー。光の3原色で表現。
		float pad1 = 0.0f;
	};

	struct PointLight {
		Vector3 m_position;//ライトの位置。3要素のベクトルで表現。
		float pad0 = 0.0f;//パティング。
		Vector3 m_color;//ライトのカラー。光の3原色で表現。
		float m_renge = 0.0f;//影響範囲。単位はメートル。
	};

	struct SpotLight {
		Vector3 m_position;//ライトの位置。3要素のベクトルで表現。
		float pad0 = 0.0f;
		Vector3 m_color;//ライトのカラー。光の3原色で表現。
		float m_range = 0.0f;//影響範囲。単位はメートル。
		Vector3 m_direction;//放射方向。3要素のベクトルで表現。
		float m_angle = 0.0f;//放射角度。
	};

	struct HemisphereLight {
		Vector3 m_groundColor;
		float pad0 = 0.0f;
		Vector3 m_skyColor;
		float pad1 = 0.0f;
		Vector3 m_groundNormal;
	};

	/// <summary>
	/// 屋内ボリュームの最大数。建物ごとに1つ使う。
	/// </summary>
	/// <remark>
	/// model.fx の INDOOR_VOLUME_MAX と必ず同じ数にすること。
	/// 増やすと定数バッファが1つにつき32バイト増える(上限に対しては誤差)。
	/// </remark>
	static const int kIndoorVolumeMax = 4;

	/// <summary>
	/// 屋内ボリューム。この箱の中だけベース光(方向光・環境光・半球光)を弱める。
	/// </summary>
	/// <remark>
	/// シーン全体で持つ。モデルごとに持たせると、床は暗いのに手に持った銃だけ
	/// 明るいといったことが起きるため、ここで一括して管理する。
	/// ポイントライトとスポットライトは弱めないので、屋内では手持ちライトと
	/// 据え置きの明かりだけが頼りになる。
	/// 並びは model.fx の IndoorVolume と合わせてあるので、順番を入れ替えないこと。
	/// 定数バッファの並びに影響するので SceneLight の中には入れないこと。
	/// </remark>
	struct IndoorVolume
	{
		Vector3 boxMin = Vector3::Zero;	//! 箱の最小座標(ワールド)。
		float feather = 300.0f;			//! 箱の端から内側へこの距離で倍率が変わり切る。
		Vector3 boxMax = Vector3::Zero;	//! 箱の最大座標(ワールド)。
		float scale = 1.0f;				//! 箱の中でのベース光の倍率。1.0で無効。
	};

	struct SceneLight
	{
		DirectionLight directionLig;
		Vector3 eyePos;
		float pad0 = 0.0f;
		Vector3 ambientLight;
		float pad1 = 0.0f;
		PointLight pointLig[10];
		SpotLight spotLig[10];
		HemisphereLight hemiLig;
		float pad2 = 0.0f;
		Matrix mLVP;//ライトビュー投影行列。
	};

		//ライトクラス。
		class Light :public IGameObject
		{
		public:

			void Init();
			void LightCameraUpdate();


			/// <summary>
			/// ディレクションライトを設定する
			/// </summary>
			/// <param name="direction">ライトの方向</param>
			/// <param name="color">ライトの色</param>
			void SetDirectionLight(Vector3 direction, Vector3 color)
			{
				m_SceneLight.directionLig.m_direction = direction;
				m_SceneLight.directionLig.m_direction.Normalize();
				m_SceneLight.directionLig.m_color = color;
			}

			/// <summary>
			/// 環境光を設定する
			/// </summary>
			/// <param name="ambient">環境光</param>
			void SetAmbient(Vector3 ambient)
			{
				m_SceneLight.ambientLight = ambient;
			}

			/// <summary>
			/// ポイントライトを設定する
			/// </summary>
			/// <param name="num">ライト番号</param>
			/// <param name="position">ライトの位置</param>
			/// <param name="color">ライトのカラー</param>
			/// <param name="range">ライトの影響範囲</param>
			void SetPointLight(int num, Vector3 position, Vector3 color, float range)
			{
				m_SceneLight.pointLig[num].m_position = position;
				m_SceneLight.pointLig[num].m_color = color;
				m_SceneLight.pointLig[num].m_renge = range;
			}

			/// <summary>
			/// スポットライトを設定する
			/// </summary>
			/// <param name="num">ライト番号</param>
			/// <param name="position">ライトの位置</param>
			/// <param name="color">ライトのカラー</param>
			/// <param name="range">ライトの影響範囲</param>
			/// <param name="direction">ライトの放射方向</param>
			/// <param name="angle">ライトの放射角度</param>
			void SetSpotLight(int num, Vector3 position, Vector3 color, float range, Vector3 direction, float angle)
			{
				m_SceneLight.spotLig[num].m_position = position;
				m_SceneLight.spotLig[num].m_color = color;
				m_SceneLight.spotLig[num].m_range = range;
				m_SceneLight.spotLig[num].m_direction = direction;
				m_SceneLight.spotLig[num].m_direction.Normalize();
				m_SceneLight.spotLig[num].m_angle = angle;
			}

			/// <summary>
			/// 半球ライトを設定する
			/// </summary>
			/// <param name="groundColor">地面色</param>
			/// <param name="skyColor">天球色</param>
			/// <param name="groundNormal">地面の法線</param>
			void SetHemLight(Vector3 groundColor, Vector3 skyColor, Vector3 groundNormal)
			{
				m_SceneLight.hemiLig.m_groundColor = groundColor;
				m_SceneLight.hemiLig.m_skyColor = skyColor;
				m_SceneLight.hemiLig.m_groundNormal = groundNormal;
			}

			/// <summary>
			/// ライトカメラの注視点を設定する
			/// </summary>
			/// <param name="target">注視点</param>
			void SetLightCameraTarget(Vector3 target)
			{
				m_lightCameraTarget = target;
				m_lightCamera.SetTarget(m_lightCameraTarget);
			}

			SceneLight& GetLight()
			{
				return m_SceneLight;
			}

			/// <summary>
			/// 屋内ボリュームをすべて差し替える。
			/// </summary>
			/// <param name="pVolumes">屋内ボリュームの配列。</param>
			/// <param name="count">配列の数。kIndoorVolumeMax を超えたぶんは捨てる。</param>
			void SetIndoorVolumes(const IndoorVolume* pVolumes, int count)
			{
				if (count > kIndoorVolumeMax)
					count = kIndoorVolumeMax;

				if (count < 0)
					count = 0;

				for (int i = 0; i < count; i++)
					m_indoorVolumes[i] = pVolumes[i];

				m_indoorVolumeCount = count;
			}

			/// <summary>
			/// 屋内ボリュームの配列の先頭を取得する。
			/// </summary>
			const IndoorVolume* GetIndoorVolumes() const
			{
				return m_indoorVolumes;
			}

			/// <summary>
			/// 設定されている屋内ボリュームの数を取得する。
			/// </summary>
			int GetIndoorVolumeCount() const
			{
				return m_indoorVolumeCount;
			}

			/// <summary>
			/// ライトカメラを取得 
			/// </summary>
			Camera& GetLightCamera()
			{
				return m_lightCamera;
			}

		private:
			SceneLight m_SceneLight;
			IndoorVolume m_indoorVolumes[kIndoorVolumeMax];	//! 屋内ボリューム(シーン全体で共有)。
			int m_indoorVolumeCount = 0;					//! 実際に使っている数。
			Camera m_lightCamera;			//ライトカメラ
			Vector3 m_lightCameraTarget;	//ライトカメラの注視点

		};
	}



