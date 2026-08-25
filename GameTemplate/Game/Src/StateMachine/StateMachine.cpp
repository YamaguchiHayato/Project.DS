#include "stdafx.h"
#include "StateMachine.h"
#include "NullState.h"
#include "Src/Actor/Actor.h"

/* @def
 * @brief テンプレートのマクロ。
 */
#define CLASS_T template<class CharacterTemplete>

 /* @def
  * @brief テンプレート名のマクロ。
  */
#define T_NAME CharacterTemplete

namespace nsApp
{
	namespace nsState
	{
		CLASS_T
		StateMachine<T_NAME>::StateMachine(T_NAME* pOwner) : pOwner_(pOwner)
		{
			/* nullステートの作成。*/
			pCurrentState_ = new NullState<T_NAME>();

			/* nullステートの初期化。*/
			pCurrentState_->Register(pOwner_, this);
		}


		CLASS_T
		StateMachine<T_NAME>::~StateMachine()
		{
			if (pCurrentState_ != nullptr)
			{
				/* ステートを終了する。*/
				pCurrentState_->Exit();
				/* ステートの削除。*/
				delete pCurrentState_;
			}
		}


		CLASS_T
		void StateMachine<T_NAME>::ChangeState(IState<T_NAME>* pNewState)
		{
			/* nullステートの作成。*/
			if (pNewState == nullptr)
				pNewState = new NullState<T_NAME>();

			/* ステートマシーンが更新中の場合、次のステートを設定する。*/
			if (bIsUpdating_)
			{
				if (pNextState_ != nullptr)
				{
					delete pNextState_;
					pNextState_ = nullptr;
				}

				pNextState_ = pNewState;
				return;
			}

			/* 現在のステートを終了する。*/
			if (pCurrentState_ != nullptr)
			{
				pCurrentState_->Exit();
				delete pCurrentState_;
				pCurrentState_ = nullptr;
			}

			/* 新しいステートを設定する。*/
			pCurrentState_ = pNewState;
			pCurrentState_->Register(pOwner_, this);
			pCurrentState_->Enter();
		}


		CLASS_T
		void StateMachine<T_NAME>::Update()
		{
			/* ステートマシーンが更新中であることを示すフラグを設定する。*/
			bIsUpdating_ = true;

			/* 現在のステートを更新する。*/
			if (pCurrentState_ != nullptr)
				pCurrentState_->Update();

			/* ステートマシーンが更新中でないことを示すフラグを解除する。*/
			bIsUpdating_ = false;

			/* 次のステートが設定されている場合、ステートを変更する。*/
			if (pNextState_ != nullptr)
			{
				IState<T_NAME>* pNextState = pNextState_;
				pNextState_ = nullptr;
				ChangeState(pNextState);
			}
		}

		/* Actorクラスに対してテンプレートの使用可能にする。*/
		template class StateMachine<nsApp::nsActor::Actor>;
	}
}