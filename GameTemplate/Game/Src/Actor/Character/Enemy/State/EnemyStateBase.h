#pragma once

#include "Src/StateMachine/IState.h"
#include "Src/Actor/Actor.h"
#include "Src/Actor/Character/Enemy/IEnemy.h"
#include "Src/Actor/Character/Enemy/Transition/EnEnemyState.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   EnemyStateBase.h
		 * @brief  敵ステートの流れを固定する基底（Template Method）。
		 * @details Enter で IEnemy へ一度キャストし、Update では再キャストしない。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/26
		 */
		class EnemyStateBase : public nsState::IState<Actor>
		{
		public:
			virtual ~EnemyStateBase() = default;

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
			 * @return 節として使う EnEnemyState。
			 */
			virtual EnEnemyState GetStateKind() const = 0;

			/**
			 * @brief 入室時の差分処理。
			 */
			virtual void OnEnter() = 0;

			/**
			 * @brief 更新時の差分処理（遷移しなかったとき）。
			 */
			virtual void OnUpdate() = 0;


		protected:
			IEnemy* pEnemy_ = nullptr; //! Enter でだけセットする所有者。
		};
	}
}