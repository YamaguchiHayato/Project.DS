#pragma once
/**
 * @file   DebugShootingRangeScene.h
 * @brief  射撃場デバッグシーン。
 * @author Yamaguchi Hayato
 * @date   2026/08/20
 */

#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsScene
	{
		/**
		 * @file   DebugShootingRangeScene.h
		 * @brief  銃テスト用の空シーン。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/20
		 */
		class DebugShootingRangeScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			DebugShootingRangeScene() = default;
			virtual ~DebugShootingRangeScene() = default;


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
			FontRender stTitleFont_;	//! 見出し表示。
			FontRender stBackFont_;		//! 戻る項目。
			bool bWasPressEsc_ = false;	//! 前フレームでESCが押されていたか。
		};
	}
}