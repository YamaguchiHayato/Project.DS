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
		class KeyConfigUI;
		class VideoScreen;
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
			nsUI::KeyConfigUI* pKeyConfig_ = nullptr; //! キーコンフィグUI。
			nsUI::VideoScreen* pVideoScreen_ = nullptr; //! 動画の再生画面。
		};
	}
}