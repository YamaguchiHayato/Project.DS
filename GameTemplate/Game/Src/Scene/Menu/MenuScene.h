#pragma once
#include "Src/Scene/IScene.h"
#include "Src/UI/Title/TitleUI.h"

namespace nsApp
{
	namespace nsUI
	{
		class TitleStage;
	}

	namespace nsScene
	{
		/**
		 * @file   MenuScene.h
		 * @brief  メニューシーン。START / OPTION / GITHUB / END を扱う。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/03
		 */
		class MenuScene : public IScene
		{
		public:
			/* コンストラクトデストラクタ。*/
			MenuScene() = default;
			virtual ~MenuScene();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override {}


		private:
			/**
			 * @brief メニューで決まった行き先を実行する。
			 * @param enCommand 決まった行き先。
			 */
			void ApplyMenuCommand(nsUI::EnTitleCommand enCommand);


		private:
			nsUI::TitleUI* pMenuUI_ = nullptr; //! メニューの2D表示。今は TitleUI のメニュー段階を使う。
			nsUI::TitleStage* pTitleStage_ = nullptr; //! メニューに立たせるモデル。
		};
	}
}