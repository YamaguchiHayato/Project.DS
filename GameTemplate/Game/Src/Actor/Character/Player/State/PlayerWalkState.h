#pragma once
#include "Src/Actor/Character/Player/State/PlayerStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerWalkState.h
		 * @brief  操作キャラの歩きステート。
		 * @details Player を所有者とする。
		           流れは PlayerStateBase。差分は OnEnter / OnUpdate のみ。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16
		 */
		class PlayerWalkState : public PlayerStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return Walk。
			 */
			EnPlayerState GetStateKind() const override;

			/**
			 * @brief 歩き開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief 歩き更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}