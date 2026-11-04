#pragma once
#include "SettingGroup.h"

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @brief グラフィックスの項目。VideoSetting.cpp の表と同じ順にすること。
		 */
		enum class VideoItem : unsigned char
		{
			Brightness,	//! 画面の明るさ。倍率。
			WindowSize,	//! ウィンドウサイズ。0 1280x720 / 1 1600x900 / 2 1920x1080。
			MotionBlur,	//! モーションブラー。0 OFF / 1 ON。
			FrameRate,	//! フレームレート上限。0 30 / 1 60 / 2 無制限。
			Count
		};

		/**
		 * @file VideoSetting.h
		 * @brief グラフィックスの設定。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/04: 新規作成日。
		 */
		class VideoSetting : public SettingGroup<VideoItem>
		{
		public:
			VideoSetting();

			/**
			 * @brief 今のウィンドウ幅を返す。
			 * @return 幅。
			 */
			unsigned int GetWindowWidth() const;

			/**
			 * @brief 今のウィンドウ高さを返す。
			 * @return 高さ。
			 */
			unsigned int GetWindowHeight() const;

			/**
			 * @brief 幅と高さから候補を選ぶ。候補に無いサイズは無視する。
			 * @param width 幅。
			 * @param height 高さ。
			 */
			void SetWindowSize(unsigned int width, unsigned int height);
		};
	}
}