#pragma once
/**
 * @file   TitleUI.h
 * @brief  タイトルUIクラス。
 * @author Yamaguchi Hayato
 * @date   2026/08/18
 * @details 過去タイトルからクラスを引用予定。
 */

namespace nsApp
{
	namespace nsUI
	{
		class TitleUI : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			TitleUI() = default;
			virtual ~TitleUI() = default;


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
			SpriteRender stTitleSprite_;	//! タイトル画像。
			FontRender stInGameFont_;		//! InGame項目。
			FontRender stDebugFont_;		//! Debug項目。
		};
	}
}