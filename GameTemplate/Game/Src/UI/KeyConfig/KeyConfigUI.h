#pragma once

/* todo 設計面を相談する必要あり。今は仮初でクラスを作成。*/

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @file KeyConfigUI.h
		 * @brief キー割り当ての一覧。表示だけを行う。
		 * @autor Yamaguchi Hayato
		 * @date 2026/10/21: 新規作成日。
		 */
		class KeyConfigUI : public IGameObject
		{
		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		public:
			/**
			 * @brief キーコンフィグUIが閉じられたかどうかを返す。
			 * @return 閉じられたら true。
			 */
			bool IsClose() const
			{
				return close_;
			}

		private:
			static constexpr int fontCount_ = 25; //! 表示できる行数。
			FontRender font_[fontCount_]; //! 1行ごとの文字。
			int selected_ = 0;		//! 今選んでいる行。
			bool wasUp_ = false;	//! 前フレームで上キーが押されていたか。
			bool wasDown_ = false;	//! 前フレームで下キーが押されていたか。
			bool wasEnter_ = false;//! 前フレームでEnterが押されていたか。
			bool waiting_ = false;	//! 次の入力を待っているか。
			bool wasBack_ = false;	//! 前フレームでBackspaceが押されていたか。
			bool wasEscape_ = false; //! 前フレームでEscapeが押されていたか。
			bool close_ = false; //! 一覧を閉じるか。
		};
	}
}