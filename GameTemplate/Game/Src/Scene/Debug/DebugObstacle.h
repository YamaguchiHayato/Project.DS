#pragma once

namespace nsApp
{
	namespace nsScene
	{
		/**
		 * @file   DebugObstacle.h
		 * @brief  視線検証用の静的遮蔽物。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/21
		 */
		class DebugObstacle : public IGameObject
		{
		public:
			DebugObstacle() = default;
			virtual ~DebugObstacle();

		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Render(RenderContext& rc) override;

			/**
			 * @brief 位置と全体サイズを設定する。
			 * @param vPos 中心座標。
			 * @param vSize 全体サイズ（幅・高さ・奥行き）。見た目とPhysicsStaticのBoxで同じ値。
			 */
			void SetTransform(const Vector3& vPos, const Vector3& vSize);

		private:
			/**
			 * @brief モデルを作る。
			 */
			void CreateModel();
			
			/**
			 * @brief PhysicsStaticObjectのBoxを作る。
			 */
			void ApplyTransform();

			/**
			 * @brief モデルのスケールを計算する。
			 * @return モデルのスケールの値。
			 */
			Vector3 CalcModelScale() const;


		private:
			PhysicsStaticObject stStaticObject_; //! 静的Boxコライダ。
			ModelRender stModel_; //! 見た目用モデル。
			Vector3 vPosition_ = Vector3::Zero;
			Vector3 vSize_ = Vector3::Zero;
			float fModelBaseSize_ = 500.0f; //! Block.tkm の基準1辺。
		};
	}
}