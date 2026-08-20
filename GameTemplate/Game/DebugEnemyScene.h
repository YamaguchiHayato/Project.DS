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
			void Render(RenderContext& rc) override {}


		private:
			/**
			 * @brief カメラを初期化する。
			 */
			void InitCamera();


		private:
			nsActor::DummyPlayer* pDummyPlayer_ = nullptr; //! 仮プレイヤー。
			nsActor::CommonEnemy* pCommonEnemy_ = nullptr; //! 雑魚敵。
		};
	}
}