#include "pch.h"
#include "ControlSetting.h"
#include <iterator>

namespace
{
	using nsApp::nsData::SettingRange;
	using nsApp::nsData::ControlItem;

	/* 名前, 最小, 最大, 刻み, 初期値。ControlItem と同じ順。 */
	const SettingRange CONTROL_TABLE[] =
	{
		{ "sensitivity",	0.1f,	2.0f,	0.1f,	1.0f },
		{ "adsSensitivity",	0.1f,	2.0f,	0.1f,	1.0f },
		{ "adsToggle",		0.0f,	1.0f,	1.0f,	0.0f },
		{ "vibration",		0.0f,	1.0f,	1.0f,	1.0f },
	};

	/* 列挙と表の行数がずれたらビルドエラーにする。 */
	static_assert(std::size(CONTROL_TABLE) == static_cast<size_t>(ControlItem::Count), "CONTROL_TABLE と ControlItem の数が違う");
}


namespace nsApp
{
	namespace nsData
	{
		ControlSetting::ControlSetting() : SettingGroup(CONTROL_TABLE){}
	}
}