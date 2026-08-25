#pragma once

namespace nsApp
{
	namespace nsSystem
	{
		/**
		 * @file   GamePause.h
		 * @brief  ゲーム全体のポーズ状態を1箇所で持つ。各GO(Player/敵/EnemyDirector)は
		 *         Update冒頭で IsGamePaused() を見て、ポーズ中は更新をスキップする。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */

		//! ポーズ中かどうか。
		bool IsGamePaused();

		//! ポーズ状態を明示的に設定する(シーン開始時のリセット等)。
		void SetGamePaused(bool bPaused);

		//! ポーズのON/OFFを切り替える。
		void ToggleGamePaused();
	}
}
