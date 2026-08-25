#pragma once
/**
 * @file   DebugSelectScene.h
 * @brief  デバッグ項目を選択するシーン。
 * @author Yamaguchi Hayato
 * @date   2026/08/19
 */

#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsScene
	{
		/**
		 * @file   DebugSelectScene.h
		 * @brief  デバッグモードの選択画面。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/19
		 */
		class DebugSelectScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			DebugSelectScene() = default;
			virtual ~DebugSelectScene() = default;


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
			FontRender stTitleFont_;			//! 見出し表示。
			FontRender stEnemySoloFont_;		//! 敵AIテスト項目。
			FontRender stShootingRangeFont_;	//! 射撃場テスト項目。
			FontRender stBackFont_;				//! 戻る項目。
			bool bWasPress1_ = false;			//! 前フレームでキー1が押されていたか。
			bool bWasPress2_ = false;			//! 前フレームでキー2が押されていたか。
			bool bWasPressEsc_ = false;			//! 前フレームでESCが押されていたか。
		};
	}
}