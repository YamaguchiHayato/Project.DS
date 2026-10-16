#pragma once
#include "Src/Actor/Character/Player/State/PlayerStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerRunState.h
		 * @brief  操作キャラの走りステート。
		 * @details Player を所有者とする。
		           流れは PlayerStateBase。差分は OnEnter / OnUpdate のみ。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16: 新規作成日。
		 */
		class PlayerRunState : public PlayerStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return Run。
			 */
			EnPlayerState GetStateKind() const override;

			/**
			 * @brief 走り開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief 走り更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}