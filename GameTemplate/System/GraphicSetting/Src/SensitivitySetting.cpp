#include "pch.h"
#include "SensitivitySetting.h"

namespace
{
	const auto DEFAULT_SENSITIVITY = 1.0f; //! デフォルトのマウス感度。
}


namespace nsApp
{
	namespace nsData
	{
		void SensitivitySetting::FillDefault()
		{
			sensitivity_ = DEFAULT_SENSITIVITY;
		}
	}
}
