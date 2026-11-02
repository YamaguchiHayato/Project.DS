#pragma once
#include "FOVSetting.h"


namespace nsApp
{
	namespace nsData
	{
		/**
		 * @fine GraphicsSetting.h 
		 * @brief: 画角、感度、明るさ、ウィンドウサイズを所持。
		 * @details: Get()を返す。
		 * @author: Yamaguchi Hayato
		 * @date: 2026/10/29
		 */
		class GraphicsSetting
		{
		public:
			/**
			 * @brief 設定の実態を返す。
			 * @return Singleton。
			 */
			static GraphicsSetting& Get()
			{
				static GraphicsSetting instance;
				return instance;
			}

			/**
			 * @brief 起動時の値を返す。
			 */
			void FillDefault();

			/**
			 * @brief 設定用のTSVファイルを読み込む。
			 * @return 設定結果。
			 */
			bool Load();

			/**
			 * @brief 設定項目を保存する。
			 * @return 保存結果。
			 */
			bool Save();

			/**
			 * @brief 
			 * @param fov 
			 */
			void SetFOV(float fov);

			/**
			 * @brief 
			 * @param sensitivity 
			 */
			void SetSensitivity(float sensitivity);

			/**
			 * @brief 
			 * @param brightness 
			 */
			void SetBrightness(float brightness);

			/**
			 * @brief ウィンドウサイズを設定する。
			 * @param width 横幅。 
			 * @param height 縦幅。
			 */
			void SetWindowSize(unsigned int width, unsigned int height);

			/**
			 * @brief 画角を返す。
			 * @return 画角。
			 */
			inline float GetFOV()const
			{
				return fov_.Get();
			}

			/**
			 * @brief マウス感度を返す。
			 * @return 感度。
			 */
			inline float GetSensitivity()const
			{
				return sensitivity_;
			}

			/**
			 * @brief 画面の明るさを返す。
			 * @return 明るさ。
			 */
			inline float GetBrightness()const
			{
				return brightness_;
			}

			/**
			 * @brief ウィンドウ幅を返す。
			 * @return ウィンドウ幅。
			 */
			inline unsigned int GetWindowWidth()const
			{
				return windowWidth_;
			}

			/**
			 * @brief ウィンドウ高さを返す。
			 * @return ウィンドウ高さ。
			 */
			inline unsigned int GetWindowHeight()const
			{
				return windowHeight_;
			}


		private:
			float sensitivity_ = 1.0f; //! マウス感度の倍率。
			float brightness_ = 1.0f; //! 画面の明るさ。
			unsigned int windowWidth_ = 1920; //! ウィンドウの幅。
			unsigned int windowHeight_ = 1080; //! ウィンドウの高さ。
			FOVSetting fov_; //! 画角の設定を管理。
		};
	}
}