#pragma once

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file SensitivitySetting.h
		 * @brief マウス感度を持つ。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/02
		 */
		class SensitivitySetting
		{
		public:
			/**
			 * @brief デフォルトの値を設定する。
			 */
			void FillDefault();

			/**
			 * @brief マウス感度を設定する。
			 * @param sensitivity 感度。
			 */
			inline void Set(float sensitivity)
			{
				sensitivity_ = sensitivity;
			}

			/**
			 * @brief マウス感度を受け取る。
			 * @return 受け取ったマウス感度。
			 */
			inline float Get() const
			{
				return sensitivity_;
			}


		private:
			float sensitivity_ = 1.0f; //! マウス感度の倍率。
		};
	}
}