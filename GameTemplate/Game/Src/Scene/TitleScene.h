#pragma once
/**
 * @file   TitleScene.h
 * @brief  タイトルシーン管理クラス。
 * @author Yamaguchi Hayato
 * @date   2026/08/18
 */

#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsUI {
		class TitleUI;
	}

	namespace nsScene
	{
		class TitleScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			TitleScene() = default;
			virtual ~TitleScene();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override {}


		private:
			nsUI::TitleUI* pTitleUI_ = nullptr; //! タイトル本体。
			bool bWasPress1_ = false;			//! 前フレームでキー1が押されていたか。
			bool bWasPress2_ = false;			//! 前フレームでキー2が押されていたか。
		};
	}
}