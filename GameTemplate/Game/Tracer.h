#pragma once

namespace nsApp
{
	namespace nsWeapon
	{
		/**
		 * @file   Tracer.h
		 * @brief  ヒットスキャン射撃の「見せるためだけ」の曳光弾(トレーサー)。
		 *         銃口→着弾点(または最大射程)へ細長いモデルを一瞬だけ表示し、すぐ消える。
		 *         当たり判定はしない(命中判定はPlayer側のレイで別途行う)。
		 * @author Izumida Kiryu
		 * @date   2026/08/20
		 */
		class Tracer : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			Tracer() = default;
			~Tracer() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;

			/**
			 * @brief 表示する区間を設定する。NewGOした直後(Startより前)に呼ぶ。
			 * @param vStart 始点(銃口)。
			 * @param vEnd   終点(着弾点または最大射程の位置)。
			 */
			void Setup(const Vector3& vStart, const Vector3& vEnd);


		private:
			ModelRender stModel_;							//! トレーサーのモデル(細長く伸ばして使う)。
			Vector3 vStart_ = Vector3::Zero;		//! 始点(銃口)。
			Vector3 vEnd_ = Vector3::Zero;		//! 終点(着弾点/最大射程)。
			float fLifeTimer_ = 0.0f;					//! 残り表示時間(秒)。
		};
	}
}
