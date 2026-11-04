#pragma once

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @brief 文字の横の揃え。
		 */
		enum class TextAlign : unsigned char
		{
			Left,	//! 左揃え。
			Center,	//! 中央揃え。
			Right	//! 右揃え。
		};

		/**
		 * @file   OptionCanvas.h
		 * @brief  設定画面の枠と文字をまとめて描く。
		 * @details 同じ Sprite を1フレームで何度も Draw すると、定数バッファが最後の値で上書きされる。
		 *          そのため枠と文字を最大数ぶん持ち、毎フレーム先頭から順に割り当てる。
		 *          枠の画像は1枚だけ読み、全部の枠で共有する。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 新規作成日。
		 */
		class OptionCanvas : public IRender
		{
		public:
			/**
			 * @brief 枠の画像を読み、枠と文字を準備する。
			 * @return 準備できたら true。
			 */
			bool Start();

			/**
			 * @brief フレームの描き始め。割り当てを先頭に戻し、2D描画に登録する。
			 * @details 文字より先に登録するため、Render の最初に呼ぶこと。
			 */
			void Begin();

			/**
			 * @brief 枠を1つ描く。
			 * @param center 中心。
			 * @param size 幅と高さ。
			 * @param color 乗算カラー。
			 */
			void DrawBox(const Vector2& center, const Vector2& size, const Vector4& color);

			/**
			 * @brief 文字を1つ描く。縦は position.y を中心にする。
			 * @param rc 描画コンテキスト。
			 * @param text 文字。
			 * @param position 位置。横は align に従う。
			 * @param scale 大きさ。
			 * @param color 色。
			 * @param align 横の揃え。
			 */
			void DrawString(RenderContext& rc, const wchar_t* text, const Vector2& position, float scale, const Vector4& color, TextAlign align);

			/**
			 * @brief 割り当てた枠を描く。エンジンの2D描画から呼ばれる。
			 * @param rc 描画コンテキスト。
			 */
			void OnRender2D(RenderContext& rc) override;


		private:
			static constexpr int MAX_BOX = 64; //! 1フレームに描ける枠の数。
			static constexpr int MAX_TEXT = 64; //! 1フレームに描ける文字の数。

			Texture texture_; //! 枠の画像。全部の枠で共有する。
			Sprite box_[MAX_BOX]; //! 枠。
			FontRender text_[MAX_TEXT]; //! 文字。
			int boxCount_ = 0; //! このフレームで使った枠の数。
			int textCount_ = 0; //! このフレームで使った文字の数。
		};
	}
}