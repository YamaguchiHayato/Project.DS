#include "stdafx.h"
#include "PlayerRunState.h"
#include "Src/Actor/Character/Player/Player.h"

namespace nsApp
{
	namespace nsActor
	{
		EnPlayerState PlayerRunState::GetStateKind() const
		{
			/* 樹の節は Run。*/
			return EnPlayerState::Run;
		}


		void PlayerRunState::OnEnter()
		{
			/* 走りアニメを再生する。*/
			pPlayer_->PlayAnimation(ANIM_LIST::Run);
		}


		void PlayerRunState::OnUpdate()
		{
			/* 走り中の仕事は無い。遷移は基底に任せる。*/
		}
	}
}