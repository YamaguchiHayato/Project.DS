#pragma once

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file FovSetting.h
		 * @brief 画角を持つ。
		 * @details 画角とFOVは本来、別の扱いだが、このゲームに限っては同じ存在として扱えるため、統一。
		 * @author Yamaguchi Hayato
		 * @date 2026/10/30
		 */
		class FOVSetting
		{
		public:
			/**
			 * @brief エラー時や起動時に挿入するデフォルト値。
			 */
			void FillDefault();


			/**
			 * @brief FOV値を設定する。
			 * @param fov FOV値。0.0f～180.0fの範囲で設定すること。
			 */
			inline void Set(float fov)
			{
				fov_ = fov;
			}


			/**
			 * @brief FOV値を取得する。
			 * @return FOV値。
			 */
			inline float Get() const
			{
				return fov_;
			}


		private:
			float fov_ = 60.0f; //! 画角。度。
		};
	}
}