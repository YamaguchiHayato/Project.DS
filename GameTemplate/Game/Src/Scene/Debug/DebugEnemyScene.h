#pragma once
#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsActor
	{
		class DummyPlayer;
		class CommonEnemy;
	}

	namespace nsScene
	{
		class DebugObstacle;

		/**
		 * @file   DebugEnemyScene.h
		 * @brief  敵AIテスト用デバッグシーン。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 */
		class DebugEnemyScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			DebugEnemyScene() = default;
			virtual ~DebugEnemyScene();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		private:
			/**
			 * @brief カメラを初期化する。
			 */
			void InitCamera();

			/**
			 * @brief デバッグ表示を更新する。
			 */
			void UpdateDebugFont();

			/**
			 * @brief プレイヤー追従カメラを更新する。
			 * @details プレイヤーの位置に応じてカメラを追従させる。
			 */
			void UpdateFollowCamera();

			/**
			 * @brief デバッグ用の敵ダメージ処理。
			 */
			void DebugDamageEnemy();


		private:
			nsActor::DummyPlayer* pDummyPlayer_ = nullptr; //! 仮プレイヤー。
			nsActor::CommonEnemy* pCommonEnemy_ = nullptr; //! 雑魚敵。
			FontRender stDebugFont_; //! デバッグ情報表示。
			DebugObstacle* pObstacle_ = nullptr; //! 視線検証用の壁。
			wchar_t aDebugText_[96] = {}; //! （64から96へ広げる）
			bool bWasPressEsc_ = false; //! 前フレームでESCが押されていたか。
			bool bWasPressT_ = false; // ! 前フレームでTが押されていたか。
		};
	}
}