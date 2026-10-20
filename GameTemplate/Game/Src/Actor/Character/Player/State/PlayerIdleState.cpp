#include "stdafx.h"
#include "PlayerIdleState.h"
#include "Src/Actor/Character/Player/Player.h"

namespace nsApp
{
	namespace nsActor
	{
		EnPlayerState PlayerIdleState::GetStateKind() const
		{
			/* 樹の節は Idle。*/
			return EnPlayerState::Idle;
		}


		void PlayerIdleState::OnEnter()
		{
			/* 待機アニメを再生する。*/
			pPlayer_->PlayAnimation(ANIM_LIST::Idle);
		}


		void PlayerIdleState::OnUpdate()
		{
			/* 待機中の仕事は無い。遷移は基底に任せる。*/
		}
	}
}