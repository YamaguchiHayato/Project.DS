#pragma once

#include "Src/StateMachine/IState.h"

namespace nsApp
{
	namespace nsState
	{
		template<class CharacterTemplete>

		/**
	      * @file   StateMachine.h
		 * @brief  ステートマシンの本体クラス。
		 * @author Yamaguchi Hayato。
		 * @date   2026/03/05。
		 */
		class StateMachine
		{
		public:
			/**
			 * @brief コンストラクタ。
			 * @param pOwner ステートマシーンが持つ実態へのポインタ。
			 */
			StateMachine(CharacterTemplete* pOwner);
			virtual ~StateMachine();

			/**
			 * @brief ステートを変更する。
			 * @param pNewState 新しいステートへのポインタ。
			 */
			void ChangeState(IState<CharacterTemplete>* pNewState);

			/**
			 * @brief ステートマシーンを更新する。
			 */
			void Update();


		/* ゲッター。*/
		public:
			/**
			 * @brief 現在のステートを取得する。
			 * @return 現在のステートへのポインタ。
			 */
			inline IState<CharacterTemplete>* GetCurrentState() const
			{
				return pCurrentState_;
			}


		private:
			CharacterTemplete* pOwner_; //! ステートマシーンが持つ実態へのポインタ。
			IState<CharacterTemplete>* pCurrentState_ = nullptr; //! 現在のステートへのポインタ。
			IState<CharacterTemplete>* pNextState_ = nullptr; //! 次のステートへのポインタ。
			bool bIsUpdating_ = false; //! ステートマシーンが更新中かどうかのフラグ。
		};
	}
}