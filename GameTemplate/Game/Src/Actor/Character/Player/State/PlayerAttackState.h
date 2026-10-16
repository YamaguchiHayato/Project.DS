#pragma once
#include "Src/Actor/Character/Player/State/PlayerStateBase.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerAttackState.h
		 * @brief  操作キャラの銃戦闘ステート。
		 * @details 敵の Attack（近接アニメ）とは別。将来ここで射撃・ADS などの挙動をまとめる。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16
		 */
		class PlayerAttackState : public PlayerStateBase
		{
		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return Attack。
			 */
			EnPlayerState GetStateKind() const override;

			/**
			 * @brief 銃戦闘開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief 銃戦闘更新時の差分処理。
			 */
			void OnUpdate() override;
		};
	}
}