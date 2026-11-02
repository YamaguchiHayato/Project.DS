#pragma once
#include "FOVSetting.h"
#include "SensitivitySetting.h"
#include "BrightnessSetting.h"
#include "WindowSetting.h"


namespace nsApp
{
	namespace nsData
	{
		/**
		 * @fine GraphicsSetting.h 
		 * @brief: 画角、感度、明るさ、ウィンドウサイズを所持。
		 * @details: Get()を返す。
		 * @author: Yamaguchi Hayato
		 * @date: 2026/11/02: 最終更新日。
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
				return sensitivity_.Get();
			}

			/**
			 * @brief 画面の明るさを返す。
			 * @return 明るさ。
			 */
			inline float GetBrightness()const
			{
				return brightnessSetting_.Get();
			}

			/**
			 * @brief ウィンドウ幅を返す。
			 * @return ウィンドウ幅。
			 */
			inline unsigned int GetWindowWidth()const
			{
				return windowSetting_.GetWidth();
			}

			/**
			 * @brief ウィンドウ高さを返す。
			 * @return ウィンドウ高さ。
			 */
			inline unsigned int GetWindowHeight()const
			{
				return windowSetting_.GetHeight();
			}


		private:
			FOVSetting fov_; //! 画角の設定を管理。
			SensitivitySetting sensitivity_; //! マウス感度の設定を管理。
			BrightnessSetting brightnessSetting_; //! 画面の明るさの設定を管理。
			WindowSetting windowSetting_; //! ウィンドウの設定を管理。
		};
	}
}