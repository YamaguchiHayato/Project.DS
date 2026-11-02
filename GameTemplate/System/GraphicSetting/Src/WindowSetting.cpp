#include "pch.h"
#include "WindowSetting.h"

namespace
{
	const auto DEFAULT_SIZE_WIDTH = 1920; //! デフォルトのウィンドウ幅。
	const auto DEFAULT_SIZE_HEIGHT = 1080; //! デフォルトのウィンドウの高さ。
}

namespace nsApp
{
	namespace nsData
	{
		void WindowSetting::FillDefault()
		{
			/* デフォルトのウィンドウサイズを設定する。*/
			width_ = DEFAULT_SIZE_WIDTH;
			height_ = DEFAULT_SIZE_HEIGHT;
		}
	}
}