#pragma once

#include "Src/StateMachine/IState.h"
#include "Src/Actor/Actor.h"
#include "Src/Actor/Character/Player/Transition/EnPlayerState.h"

namespace nsApp
{
	namespace nsActor
	{
		class Player;

		/**
		 * @file   PlayerStateBase.h
		 * @brief  操作キャラステートの流れを固定する基底（Template Method）。
		 * @details Enter で Player へ一度キャストし、Update では再キャストしない。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16: 新規作成日。
		 */
		class PlayerStateBase : public nsState::IState<Actor>
		{
		public:
			virtual ~PlayerStateBase() = default;

			/**
			 * @brief 入室。キャスト・節同期のあと差分処理へ。
			 */
			void Enter() final;

			/**
			 * @brief 更新。遷移試行のあと差分処理へ。
			 */
			void Update() final;

			/**
			 * @brief 退室。キャッシュを捨てる。
			 */
			void Exit() override;


		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return 節として使う EnPlayerState。
			 */
			virtual EnPlayerState GetStateKind() const = 0;

			/**
			 * @brief 入室時の差分処理。
			 */
			virtual void OnEnter() = 0;

			/**
			 * @brief 更新時の差分処理（遷移しなかったとき）。
			 */
			virtual void OnUpdate() = 0;


		protected:
			Player* pPlayer_ = nullptr; //! Enter でだけセットする所有者。
		};
	}
}