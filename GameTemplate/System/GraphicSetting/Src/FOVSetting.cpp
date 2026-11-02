#include "pch.h"
#include "FOVSetting.h"

namespace
{
	const auto DEFAULT_FOV = 60.0f; //! デフォルトのFOV値。
}

namespace nsApp
{
	namespace nsData
	{
		void FOVSetting::FillDefault()
		{
			/* デフォルト値を設定する */
			fov_ = DEFAULT_FOV;
		}
	}
}