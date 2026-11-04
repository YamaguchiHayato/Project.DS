#pragma once
#include "SettingGroup.h"

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @brief サウンドの項目。SoundSetting.cpp の表と同じ順にすること。
		 */
		enum class SoundItem : unsigned char
		{
			Mute,	//! ミュート。0 OFF / 1 ON。
			Master,	//! マスター音量。0.0～1.0。
			Bgm,	//! BGM 音量。
			Se,		//! SE 音量。
			Count
		};

		/**
		 * @file SoundSetting.h
		 * @brief サウンドの設定。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/04: 新規作成日。
		 */
		class SoundSetting : public SettingGroup<SoundItem>
		{
		public:
			SoundSetting();
		};
	}
}