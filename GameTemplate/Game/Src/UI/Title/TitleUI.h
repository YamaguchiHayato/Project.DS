#pragma once

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @file   TitleUI.h
		 * @brief  タイトルUIクラス。
	     * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 * @TODO: UI系統のクラスは処理を細分化。
		 * @details 過去タイトルからクラスを引用予定。
		 */
		class TitleUI : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			TitleUI() = default;
			virtual ~TitleUI() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		private:
			SpriteRender stTitleSprite_; //! タイトル画像。
		};
	}
}