#pragma once
#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsUI
	{
		class OptionUI;
	}

	namespace nsScene
	{
		/**
	     * @file   OptionScene.h
		 * @brief  設定シーン。OptionUI を開く。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/03
		 */
		class OptionScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			OptionScene() = default;
			virtual ~OptionScene();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override {}


		private:
			nsUI::OptionUI* pOptionUI_ = nullptr; //! 設定の表示。
		};
	}
}