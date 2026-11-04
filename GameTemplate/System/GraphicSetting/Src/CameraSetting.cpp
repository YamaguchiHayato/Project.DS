#include "pch.h"
#include "CameraSetting.h"
#include <iterator>

namespace
{
	using nsApp::nsData::SettingRange;
	using nsApp::nsData::CameraItem;

	/* 名前, 最小, 最大, 刻み, 初期値。CameraItem と同じ順。 */
	const SettingRange CAMERA_TABLE[] =
	{
		{ "invertY",	0.0f,	1.0f,	1.0f,	0.0f },
		{ "invertX",	0.0f,	1.0f,	1.0f,	0.0f },
		{ "fov",		50.0f,	110.0f,	5.0f,	60.0f },
	};

	/* 列挙と表の行数がずれたらビルドエラーにする。 */
	static_assert(std::size(CAMERA_TABLE) == static_cast<size_t>(CameraItem::Count), "CAMERA_TABLE と CameraItem の数が違う");
}

namespace nsApp
{
	namespace nsData
	{
		CameraSetting::CameraSetting() : SettingGroup(CAMERA_TABLE) {}
	}
}