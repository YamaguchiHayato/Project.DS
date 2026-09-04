#pragma once
/**
 * @file   DebugShootingRangeScene.h
 * @brief  射撃場デバッグシーン。
 * @details Player／銃担当が、本編ステージ無しで銃の性能・命中を試す場。
 * @author Yamaguchi Hayato
 * @date   2026/08/20
 */

#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsActor
	{
		class Player;
		class CommonEnemy;
	}

	namespace nsScene
	{
		/**
		 * @file   DebugShootingRangeScene.h
		 * @brief  銃テスト用の射撃場シーン。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/20
		 */
		class DebugShootingRangeScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			DebugShootingRangeScene() = default;
			virtual ~DebugShootingRangeScene();


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
			 * @brief カメラのニア・ファーを設定する。
			 */
			void InitCamera();

			/**
			 * @brief カメラをプレイヤーへ追従させる(一人称/三人称は切り替え式)。
			 */
			void UpdateCamera();

			/* --- ここから下はデバッグ用。本編の処理とは混ぜない。--- */

			/**
			 * @brief 一人称と三人称を切り替える入力を処理する。
			 *        他プレイヤーから見たときの姿(銃を持った体)を確認するためのもの。
			 */
			void UpdateViewModeSwitch();

			/**
			 * @brief 確認用の値(視点・モデルの実測サイズ・銃の位置)をヒント表示へ書き出す。
			 */
			void UpdateDebugHint();

			/**
			 * @brief 的となる雑魚敵を奥に配置する。
			 */
			void SpawnTargetEnemies();


		private:
			nsActor::Player* pPlayer_ = nullptr; //! プレイヤー（本番と同じ）。
			nsActor::CommonEnemy* aTargetEnemies_[12] = {}; //! 奥に置く的役の雑魚敵。
			ModelRender stGroundModel_; //! 地面。
			PhysicsStaticObject stGroundCollider_; //! 地面の静的コライダ。
			FontRender stHintFont_; //! 操作ヒント。
			bool bWasPressEsc_ = false; //! 前フレームでESCが押されていたか。
			bool bWasPressViewKey_ = false; //! 前フレームで視点切り替えキーが押されていたか。
			bool bIsThirdPersonView_ = false; //! 三人称(他人から見た姿)で表示しているか。
			wchar_t wcHint_[128] = {}; //! ヒント表示の文字列(視点と実測サイズを出す)。
		};
	}
}