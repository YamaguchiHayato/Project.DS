#pragma once
#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsUI
	{
		class OptionUI;
		class TitleStage;
	}

	namespace nsScene
	{
		/**
		 * @file   OptionScene.h
		 * @brief  設定シーン。タイトルのモデルの上に OptionUI を開く。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 最終更新日。背景に TitleStage を残すようにした。
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
			nsUI::TitleStage* pTitleStage_ = nullptr; //! タイトルのモデル。暗い膜の下に残す。
		};
	}
}