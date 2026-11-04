#pragma once
#include "SettingGroup.h"

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @brief 操作の項目。ControlSetting.cpp の表と同じ順にすること。
		 */
		enum class ControlItem : unsigned char
		{
			Sensitivity,	//! マウス感度。倍率。
			AdsSensitivity,	//! 覗き込み中の感度。倍率。
			AdsToggle,		//! 覗き込み操作。0 押している間 / 1 切り替え。
			Vibration,		//! コントローラー振動。0 OFF / 1 ON。
			Count
		};

		/**
		 * @file ControlSetting.h
		 * @brief 操作の設定。キー割り当ては KeyConfig 側で持つ。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/04: 新規作成日。
		 */
		class ControlSetting : public SettingGroup<ControlItem>
		{
		public:
			ControlSetting();
		};
	}
}