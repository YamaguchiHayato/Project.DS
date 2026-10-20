#pragma once
#include "PlayerIntent.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   IPlayerController.h
		 * @brief  プレイヤーを操作する「意思の供給源」の抽象インターフェース。
		 *         Playerはこの窓口から PlayerIntent を受け取るだけで、
		 *         入力がローカル実機か・ネット受信かを一切気にしない(Strategyパターン)。
		 *         ローカル=LocalPlayerController、将来のリモート=RemotePlayerController が実装する。
		 * @author Izumida Kiryu
		 * @date   2026/08/20
		 */
		class IPlayerController
		{
		public:
			/* デストラクタ。*/
			virtual ~IPlayerController() = default;


		public:
			/**
			 * @brief このフレームの操作意図を取得する。
			 * @param stOut 充填先の PlayerIntent(呼び出し側が用意する)。
			 */
			virtual void PollIntent(PlayerIntent& stOut) = 0;
		};
	}
}
