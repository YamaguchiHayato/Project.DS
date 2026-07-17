#pragma once

#include "RenderingEngine.h";
namespace nsK2EngineLow {
	class ModelRender:public IRender
	{
	public:
		/// <summary>
		/// 通常描画用の初期化
		/// </summary>
		/// <param name="filePath">ファイルパス</param>
		/// <param name="animationClips">アニメーションクリップ</param>
		/// <param name="numAnimationClips">アニメーションクリップの数</param>
		/// <param name="enModelUpAxis">モデルの上方向</param>
		/// <param name="shadowCast">trueなら影を描画する</param>
		/// <param name="Shadowdrop">trueなら影を受ける</param>
		void Init(const char* filePath,
			AnimationClip* animationClips = nullptr,
			int numAnimationCrips = 0,
			EnModelUpAxis enModelUpAxis = enModelUpAxisZ,
			bool shadowCast = false,
			bool shadowdrop = false
		);
		
		void InitShadowCasterDrawing(const char* filePath, EnModelUpAxis enModelUpAxis);
		void InitSkyCubeModel(ModelInitData& initData);

		void InitModel(const char* filePath, EnModelUpAxis enModelUpAxis,bool shadowDrop=true);

		void Update();

		void Draw(RenderContext& rc);

		//void OnDraw(RenderContext& rc) override;
		void OnRenderModel(RenderContext& rc) override;
		void OnRenderShadowMap(RenderContext& rc, Camera& came);
		void PlayAnimation(int animNo, float interpolateTime = 0.0f)
		{
			m_animation.Play(animNo, interpolateTime);
		}

		/// <summary>
		/// アニメーションの再生中？
		/// </summary>
		bool IsPlayingAnimation() const
		{
			return m_animation.IsPlaying();
		}

		void SetPosition(const Vector3& pos)
		{
			m_position = pos;
		}
		/// <summary>
		/// 座標を設定。
		/// </summary>
		/// <param name="x">x座標</param>
		/// <param name="y">y座標</param>
		/// <param name="z">z座標</param>
		void SetPosition(float x, float y, float z)
		{
			SetPosition({ x, y, z });
		}
		/// <summary>
		/// 回転を設定。
		/// </summary>
		/// <remark>
		/// インスタンシング描画が有効の場合は、この設定は無視されます。
		/// </remark>
		/// <param name="rotation">回転。</param>
		void SetRotation(const Quaternion& rotation)
		{
			m_rotation = rotation;
		}

		const Quaternion GetRotation()
		{
			return m_rotation;
		}

		/// <summary>
		/// 拡大率を設定。
		/// </summary>
		/// <remark>
		/// インスタンシング描画が有効の場合は、この設定は無視されます。
		/// </remark>
		/// <param name="scale">拡大率。</param>
		void SetScale(const Vector3& scale)
		{
			m_scale = scale;
		}
		void SetScale(float x, float y, float z)
		{
			SetScale({ x, y, z });
		}

		/// <summary>
	/// モデルを取得。
	/// </summary>
	/// <returns>モデル</returns>
		Model& GetModel()
		{
			return m_model;
		}

		/// <summary>
		/// スケルトンの初期化。
		/// </summary>
		/// <param name="filePath">ファイルパス。</param>
		void InitSkeleton(const char* filePath);
		/// <summary>
		/// アニメーションの初期化。
		/// </summary>
		/// <param name="animationClips">アニメーションクリップ。</param>
		/// <param name="numAnimationClips">アニメーションクリップの数。</param>
		/// <param name="enModelUpAxis">モデルの上向き。</param>
		void InitAnimation(AnimationClip* animationClips,
			int numAnimationClips,
			EnModelUpAxis enModelUpAxis);

		
		float GetAnimationRatio();

		
		//座標拡大回転全てを設定
		void SetTRS(const Vector3& pos, const Quaternion& rotation, const Vector3& scale)
		{
			SetPosition(pos);
			SetRotation(rotation);
			SetScale(scale);
		}


		AnimationClip* m_animationClips = nullptr;		//アニメーションクリップ
		int m_numAnimationClips = 0;					//アニメーションの数
		Vector3 m_position = Vector3::Zero;				//座標
		Quaternion m_rotation = Quaternion::Identity;	//回転
		Vector3 m_scale = Vector3::One;					//拡大率
		EnModelUpAxis m_modelUpAxis = enModelUpAxisZ;	//モデルの上方向
		Animation m_animation;							//アニメーション	
		Model m_model;									//モデル
		Model m_shadowModel;							//影描画用モデル
		bool m_isUpdateAnimation = true;				//アニメーションを更新する？
		Skeleton m_skeleton;							//骨
		float m_animationSpeed = 1.0f;					//アニメーション再生速度
		float m_alpha = 1.0f;
};
}

