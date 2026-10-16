#include "stdafx.h"
#include "PlayerAttackState.h"
#include "Player.h"

namespace nsApp
{
	namespace nsActor
	{
		EnPlayerState PlayerAttackState::GetStateKind() const
		{
			/* 樹の節は Attack。*/
			return EnPlayerState::Attack;
		}


		void PlayerAttackState::OnEnter()
		{
			/*
			 * 銃戦闘ステートに入ったときの準備をここに書く。
			 * Attack.tka は再生しない（銃は Idle/Walk/Run の上で扱う）。
			 */
		}


		void PlayerAttackState::OnUpdate()
		{
			/*
			 * 将来：射撃入力・リロード・ADS などをここに寄せる。
			 * いまは何もしない。遷移枝もまだ張っていない。
			 */
		}
	}
}