#pragma once
#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsActor
	{
		/* 前方宣言。*/
		class Player;
	}

	namespace nsScene
	{
		/**
		 * @file   DebugPlayerScene.h
		 * @brief  プレイヤー単体テスト用デバッグシーン。
		 *         プレイヤーを生成し、追従カメラで移動・射撃・武器切り替えを確認する。
		 * @author Izumida Kiryu
		 * @date   2026/08/19
		 */
		class DebugPlayerScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			DebugPlayerScene() = default;
			virtual ~DebugPlayerScene();


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
			 * @brief カメラをプレイヤーに追従させる。
			 */
			void UpdateCamera();


		private:
			nsActor::Player* pPlayer_ = nullptr;	//! プレイヤー。
			ModelRender		stGroundModel_;			//! 地面(カメラ回転が見えるようにする目印)。
		};
	}
}
