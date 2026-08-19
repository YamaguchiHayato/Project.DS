#pragma once
#include "Src/StateMachine/StateMachine.h"


namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   Actor.h
		 * @brief  IGameObjectを継承し、必要な要素を抜き取ったクラス。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 */
		class Actor : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			Actor();
			virtual ~Actor();


		public:
			/* ライフサイクル。*/
			virtual bool Start() = 0;
			virtual void Update();
			virtual void Render(RenderContext& rc) = 0;


		protected:
			nsState::StateMachine<Actor>* pStateMachine_ = nullptr; //! 共通のステートマシーン。
		};
	}
}