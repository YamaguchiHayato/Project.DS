#pragma once

namespace nsApp
{
	namespace nsEffect
	{
		/**
		 * @file   HitEffect.h
		 * @brief  一瞬だけ表示して自動で消える発光エフェクト。マズルフラッシュや撃破の閃光に使う。
		 *         発光球(VolumePointLight)を開始→終了サイズへ補間しながら短時間で消滅する。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */
		class HitEffect : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			HitEffect() = default;
			virtual ~HitEffect() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;

			/**
			 * @brief 表示内容を設定する。NewGOした直後(Startより前)に呼ぶ。
			 * @param vPos        表示位置。
			 * @param fStartScale 開始サイズ。
			 * @param fEndScale   終了サイズ(この間を補間しながら消える)。
			 * @param fLifeSec    表示時間(秒)。
			 */
			void Setup(const Vector3& vPos, float fStartScale, float fEndScale, float fLifeSec);


		private:
			ModelRender	stModel_;						//! 発光球モデル。
			Vector3		vPos_ = { 0.0f, 0.0f, 0.0f };	//! 表示位置。
			float		fStartScale_ = 10.0f;			//! 開始サイズ。
			float		fEndScale_ = 30.0f;				//! 終了サイズ。
			float		fLife_ = 0.1f;					//! 総表示時間(秒)。
			float		fTimer_ = 0.0f;					//! 経過時間(秒)。
		};
	}
}
