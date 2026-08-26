#pragma once

namespace nsApp
{
	namespace nsItem
	{
		/**
		 * @file   Grenade.h
		 * @brief  投擲アイテム(グレネード)。視線方向へ山なりに飛び、着地または信管切れで爆発し、
		 *         周囲の敵にまとめてダメージを与える。命中判定は自前の距離判定(暫定)。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */
		class Grenade : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			Grenade() = default;
			virtual ~Grenade() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;

			/**
			 * @brief 投擲開始の位置と方向を設定する。NewGO直後(Startより前)に呼ぶ。
			 * @param vPos 投擲開始位置。
			 * @param vDir 投擲方向(正規化前でも可)。
			 */
			void Setup(const Vector3& vPos, const Vector3& vDir);


		private:
			/**
			 * @brief 爆発して周囲の敵にダメージを与える。
			 */
			void Explode();


		private:
			ModelRender stModel_;						//! 表示モデル(仮の発光球)。
			Vector3 vPos_ = Vector3::Zero;	//! 現在位置。
			Vector3 vVel_ = Vector3::Zero;	//! 速度。
			float fFuse_ = 1.5f;					//! 起爆までの残り時間(秒)。
		};
	}
}
