#include "stdafx.h"
#include "Actor.h"
#include "Src/StateMachine/StateMachine.h"

namespace nsApp
{
	namespace nsActor
	{
		Actor::Actor()
		{
			/* Actorクラスをオーナーとし、StateMachineを作成。*/
			pStateMachine_ = new nsState::StateMachine<Actor>(this);
		}


		Actor::~Actor()
		{
			/* 削除。*/
			delete pStateMachine_;
		}


		void Actor::Update()
		{
			/* ステートマシーンの更新。*/
			pStateMachine_->Update();
		}
	}
}