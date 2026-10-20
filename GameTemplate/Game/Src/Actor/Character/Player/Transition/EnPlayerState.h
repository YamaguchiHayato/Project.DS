#pragma once
#include "stdint.h"

namespace nsApp
{
	/**
	 * @enum EnPlayerState
	 * @brief プレイヤーの状態。
	 * @details プレイヤーの現在の状態を表す列挙型。
	 * @author Yamaguchi Hayato
	 * @date 2026/10/16: 新規作成日。
	 */
	enum class EnPlayerState : uint8_t
	{
		Idle,
		Walk, 
		Run,
		WinPause,
		Attack,
	};
}