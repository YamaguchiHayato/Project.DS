#pragma once

namespace nsApp
{
	namespace nsState
	{
		/* 循環を防ぐために前方宣言。*/
		template <class CharacterTemplate>
		/* ステートマシーンクラス本体。*/
		class StateMachine;

		/* ステート作成用のテンプレート。*/
		template <class CharacterTemplate>

		/**
		 * @file   IState.h
		 * @brief  ステートのインターフェース。
		 * @author Yamaguchi Hayato。
		 * @date   2026/03/05。
		 */
		class IState
		{
		protected:
			CharacterTemplate* pOwner_; //! ステートを持つ実態へのポインタ。
			StateMachine<CharacterTemplate>* pStateMachine_ = nullptr;	//! ステートマシーンへのポインタ。


		public:
			/* デストラクタ。*/
			virtual ~IState() = default;


		public:
			/**
			 * @brie f ステートを登録する。
			 * @param pOwner ステートを持つ実態へのポインタ。
			 * @param pStateMachine ステートマシーンへのポインタ。
			 */
			inline void Register(CharacterTemplate* pOwner, StateMachine<CharacterTemplate>* pStateMachine)
			{
				pOwner_ = pOwner;
				pStateMachine_ = pStateMachine;
			}


		public:
			/* ライフサイクル。 */
			virtual void Enter() = 0;
			virtual void Update() = 0;
			virtual void Exit() = 0;
			virtual bool RequestID(uint8_t& request)
			{
				return false;
			}
		};
	}
}