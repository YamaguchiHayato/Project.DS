#pragma once

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file WindowSetting.h
		 * @brief ウィンドウの幅と高さを持つ。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/02
		 */
		class WindowSetting
		{
		public:
			/**
			 * @brief デフォルトの値を設定する。
			 */
			void FillDefault();

			/**
			 * @brief ウィンドウサイズを設定する。
			 * @param width 縦。
			 * @param height 横。
			 */
			inline void Set(unsigned int width, unsigned int height)
			{
				width_ = width;
				height_ = height;
			}

			/**
			 * @brief 横幅のサイズを設定する。
			 * @param width 横幅の設定値。
			 */
			inline void SetWidth(unsigned int width)
			{
				width_ = width;
			}

			/**
			 * @brief 縦幅のサイズを設定する。
			 * @param height 縦幅の設定値。
			 */
			inline void SetHeight(unsigned int height)
			{
				height_ = height;
			}

			/**
			 * @brief 横幅のサイズを取得する。
			 * @return 横幅の取得値。
			 */
			inline unsigned int GetWidth() const
			{
				return width_;
			}

			/**
			 * @brief 縦幅のサイズを取得する。
			 * @return 縦幅の取得値。
			 */
			inline unsigned int GetHeight() const
			{
				return height_;
			}


		private:
			unsigned int width_ = 1920; //! 幅。
			unsigned int height_ = 1080; //! 高さ。
		};
	}
}