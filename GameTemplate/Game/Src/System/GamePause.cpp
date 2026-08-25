#include "stdafx.h"
#include "GamePause.h"

namespace nsApp
{
	namespace nsSystem
	{
		namespace
		{
			bool s_bPaused = false;	//! 現在ポーズ中か。
		}

		bool IsGamePaused()
		{
			return s_bPaused;
		}

		void SetGamePaused(bool bPaused)
		{
			s_bPaused = bPaused;
		}

		void ToggleGamePaused()
		{
			s_bPaused = !s_bPaused;
		}
	}
}
