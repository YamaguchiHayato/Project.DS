#include "pch.h"
#include "BrightnessSetting.h"

namespace
{
	const auto DEFAULT_BRIGHTNESS = 1.0f; //! デフォルトの明るさ。
}


namespace nsApp
{
	namespace nsData
	{
		void BrightnessSetting::FillDefault()
		{
			brightness_ = DEFAULT_BRIGHTNESS;
		}
	}
}