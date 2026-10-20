#pragma once
#include "Src/Actor/Character/Player/State/PlayerStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerIdleState.h
		 * @brief  操作キャラの待機ステート。
		 * @details Player を所有者とする。
		            流れは PlayerStateBase。差分は OnEnter / OnUpdate のみ。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16
		 */
		class PlayerIdleState : public PlayerStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return Idle。
			 */
			EnPlayerState GetStateKind() const override;

			/**
			 * @brief 待機開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief 待機更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}