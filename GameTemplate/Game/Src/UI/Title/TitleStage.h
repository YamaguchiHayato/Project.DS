#pragma once
/**
 * @file   TitleStage.h
 * @brief  タイトル画面に立たせるモデルと、そのためのカメラ・ライトを用意する。
 * @author Izumida Kiryu
 * @date   2026/10/29
 * @note   ロゴやメニューなどの2D表示は TitleUI が受け持つ。こちらは3D表示だけを受け持つ。
 *         カメラとライトはシーンをまたいで共用なので、来たときの値を控えて、抜けるときに戻す。
 */

namespace nsApp
{
	namespace nsUI
	{
		class TitleStage : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			TitleStage() = default;
			virtual ~TitleStage();


		public:
			/* ライフサイクル。*/
			/**
			 * @brief 初期化処理。
			 * @return 初期化に成功したらtrue。
			 */
			bool Start() override;

			/**
			 * @brief 更新処理。
			 */
			void Update() override;

			/**
			 * @brief 描画処理。
			 * @param rc レンダリングコンテキスト。
			 */
			void Render(RenderContext& rc) override;


		private:
			/**
			 * @brief モデルが画面の右側に来るようにカメラを置く。
			 */
			void SetUpCamera();

			/**
			 * @brief タイトル用のライトを入れる。本編の屋外設定のままだと暗くて顔が見えないため。
			 */
			void SetUpLight();


		private:
			static constexpr int iAnimationCount_ = 1;	//! 読み込むアニメーションの数。
			ModelRender stModelRender_;					//! 立たせるモデル。
			AnimationClip aAnimationClip_[iAnimationCount_];	//! 待機アニメーション。
			SceneLight stSavedLight_;					//! 来たときのライト。抜けるときに戻す。
			Vector3 vSavedCameraPosition_ = Vector3::Zero;	//! 来たときのカメラ位置。
			Vector3 vSavedCameraTarget_ = Vector3::Zero;	//! 来たときのカメラ注視点。
			float fModelSize_ = 0.0f;					//! 頂点から測ったモデルの大きさ。カメラの寄り引きの基準にする。
			float fSavedViewAngle_ = 0.0f;				//! 来たときの画角(ラジアン)。
		};
	}
}
