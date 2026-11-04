#include "pch.h"
#include "SoundSetting.h"
#include <iterator>

namespace
{
	using nsApp::nsData::SettingRange;
	using nsApp::nsData::SoundItem;

	/* 名前, 最小, 最大, 刻み, 初期値。SoundItem と同じ順。20段階。 */
	const SettingRange SOUND_TABLE[] =
	{
		{ "mute",			0.0f,	1.0f,	1.0f,	0.0f },
		{ "masterVolume",	0.0f,	1.0f,	0.05f,	1.0f },
		{ "bgmVolume",		0.0f,	1.0f,	0.05f,	1.0f },
		{ "seVolume",		0.0f,	1.0f,	0.05f,	1.0f },
	};

	/* 列挙と表の行数がずれたらビルドエラーにする。 */
	static_assert(std::size(SOUND_TABLE) == static_cast<size_t>(SoundItem::Count), "SOUND_TABLE と SoundItem の数が違う");
}


namespace nsApp
{
	namespace nsData
	{
		SoundSetting::SoundSetting() : SettingGroup(SOUND_TABLE){}
	}
}