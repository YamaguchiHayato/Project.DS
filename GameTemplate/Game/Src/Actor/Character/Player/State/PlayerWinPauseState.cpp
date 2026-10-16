#include "stdafx.h"
#include "PlayerWinPauseState.h"
#include "Player.h"

namespace nsApp
{
	namespace nsActor
	{
		EnPlayerState PlayerWinPauseState::GetStateKind() const
		{
			/* 樹の節は WinPause。*/
			return EnPlayerState::WinPause;
		}


		void PlayerWinPauseState::OnEnter()
		{
			/* リザルト用ポーズアニメを再生する。*/
			pPlayer_->PlayAnimation(ANIM_LIST::WinPause);
		}


		void PlayerWinPauseState::OnUpdate()
		{
			/* ポーズ中の仕事は無い。遷移枝は後で足す。*/
		}
	}
}