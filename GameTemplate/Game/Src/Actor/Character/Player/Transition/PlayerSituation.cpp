#include "stdafx.h"
#include "PlayerSituation.h"

namespace nsApp
{
	namespace nsActor
	{
		void PlayerSituation::Clear()
		{
			/* すべてのフラグを初期化する。*/
			moving_ = false;
			sprinting_ = false;
			wantWinPause_ = false;
			wantAttack_ = false;
		}


		uint32_t PlayerSituation::MakeFactMask() const
		{
			/* bool の事実を遷移照合用のビットに落とす。*/
			uint32_t uMask = enPlayerFact_None;

			/* 移動遷移判定。*/
			if (moving_)
				uMask |= enPlayerFact_Moving;
			else
				uMask |= enPlayerFact_NotMoving;

			/* スプリント遷移判定。*/
			if (sprinting_)
				uMask |= enPlayerFact_Sprinting;
			else
				uMask |= enPlayerFact_NotSprinting;

			/* リザルト時のポーズ要求。*/
			if (wantWinPause_)
				uMask |= enPlayerFact_WantWinPause;

			/* 攻撃判定。*/
			if (wantAttack_)
				uMask |= enPlayerFact_WantAttack;
			else
				uMask |= enPlayerFact_NotWantAttack;

			/* 遷移結果を返す。*/
			return uMask;
		}


		bool PlayerSituation::Matches(uint32_t uNeed) const
		{
			/* 現在の事実が、要求ビットをすべて含んでいるか。*/
			const uint32_t uHave = MakeFactMask();
			return (uHave & uNeed) == uNeed;
		}
	}
}