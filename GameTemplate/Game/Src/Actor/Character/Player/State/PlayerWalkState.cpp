#include "stdafx.h"
#include "PlayerWalkState.h"
#include "Src/Actor/Character/Player/Player.h"

namespace nsApp
{
	namespace nsActor
	{
		EnPlayerState PlayerWalkState::GetStateKind() const
		{
			/* 樹の節は Walk。*/
			return EnPlayerState::Walk;
		}


		void PlayerWalkState::OnEnter()
		{
			/* 歩きアニメを再生する。*/
			pPlayer_->PlayAnimation(ANIM_LIST::Walk);
		}


		void PlayerWalkState::OnUpdate()
		{
			/* 歩き中の仕事は無い。遷移は基底に任せる。*/
		}
	}
}