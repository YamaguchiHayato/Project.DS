#pragma once
#include "Src/Actor/Character/Player/State/PlayerStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerWinPauseState.h
		 * @brief  操作キャラのリザルト用ポーズステート。
		 * @details Player を所有者とする。
		 　　　　　流れは PlayerStateBase。差分は OnEnter / OnUpdate のみ。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16: 新規作成。
		 */
		class PlayerWinPauseState : public PlayerStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return WinPause。
			 */
			EnPlayerState GetStateKind() const override;

			/**
			 * @brief ポーズ開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief ポーズ更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}