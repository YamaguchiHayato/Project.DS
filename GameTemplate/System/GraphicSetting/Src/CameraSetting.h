#pragma once
#include "SettingGroup.h"

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @brief カメラの項目。CameraSetting.cpp の表と同じ順にすること。
		 */
		enum class CameraItem : unsigned char
		{
			InvertY,	//! 上下反転。0 通常 / 1 反転。
			InvertX,	//! 左右反転。0 通常 / 1 反転。
			Fov,		//! 視野角。度。
			Count
		};

		/**
		 * @file CameraSetting.h
		 * @brief カメラの設定。中身は CameraSetting.cpp の表。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/04: 新規作成日。
		 */
		class CameraSetting : public SettingGroup<CameraItem>
		{
		public:
			/**
			 * @brief カメラの設定を行う。
			 */
			CameraSetting();
		};
	}
}