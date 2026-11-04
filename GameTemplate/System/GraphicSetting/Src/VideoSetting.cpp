#include "pch.h"
#include "VideoSetting.h"
#include <iterator>

namespace
{
	using nsApp::nsData::SettingRange;
	using nsApp::nsData::VideoItem;

	/* ウィンドウサイズの候補。VideoItem::WindowSize の番号で引く。 */
	const unsigned int WINDOW_WIDTH[] = { 1280, 1600, 1920 };
	const unsigned int WINDOW_HEIGHT[] = { 720, 900, 1080 };
	const int WINDOW_COUNT = static_cast<int>(std::size(WINDOW_WIDTH));

	/* 名前, 最小, 最大, 刻み, 初期値。VideoItem と同じ順。 */
	const SettingRange VIDEO_TABLE[] =
	{
		{ "brightness",	0.2f,	2.0f,	0.1f,	1.0f },
		{ "windowSize",	0.0f,	static_cast<float>(WINDOW_COUNT - 1),	1.0f,	static_cast<float>(WINDOW_COUNT - 1) },
		{ "motionBlur",	0.0f,	1.0f,	1.0f,	1.0f },
		{ "frameRate",	0.0f,	2.0f,	1.0f,	1.0f },
	};

	/* 列挙と表の行数がずれたらビルドエラーにする。 */
	static_assert(std::size(VIDEO_TABLE) == static_cast<size_t>(VideoItem::Count), "VIDEO_TABLE と VideoItem の数が違う");
	static_assert(std::size(WINDOW_WIDTH) == std::size(WINDOW_HEIGHT), "WINDOW_WIDTH と WINDOW_HEIGHT の数が違う");
}


namespace nsApp
{
	namespace nsData
	{
		VideoSetting::VideoSetting() : SettingGroup(VIDEO_TABLE)
		{
		}


		unsigned int VideoSetting::GetWindowWidth() const
		{
			return WINDOW_WIDTH[GetIndex(VideoItem::WindowSize)];
		}


		unsigned int VideoSetting::GetWindowHeight() const
		{
			return WINDOW_HEIGHT[GetIndex(VideoItem::WindowSize)];
		}


		void VideoSetting::SetWindowSize(unsigned int width, unsigned int height)
		{
			/* 候補の中から同じサイズを探す。 */
			for (int index = 0; index < WINDOW_COUNT; index++)
			{
				if (WINDOW_WIDTH[index] == width && WINDOW_HEIGHT[index] == height)
				{
					Set(VideoItem::WindowSize, static_cast<float>(index));
					return;
				}
			}
		}
	}
}