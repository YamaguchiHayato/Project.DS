#pragma once

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file BrightnessSetting.h
		 * @brief 画面の明るさを持つ。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/02
		 */
		class BrightnessSetting
		{
		public:
			/**
			 * @brief デフォルトの値を設定する。
			 */
			void FillDefault();

			/**
			 * @brief 明るさの情報を設定する。
			 * @param brightness 設定した明るさの情報。
			 */
			inline void Set(float brightness)
			{
				brightness_ = brightness;
			}

			/**
			 * @brief 明るさの情報を取得する。
			 * @return 受け取った明るさの情報。
			 */
			inline float Get() const
			{
				return brightness_;
			}


		private:
			float brightness_ = 1.0f; //! 画面の明るさ。
		};
	}
}