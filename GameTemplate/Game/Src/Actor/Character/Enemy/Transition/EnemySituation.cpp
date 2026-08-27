#include "stdafx.h"
#include "EnemySituation.h"

namespace nsApp
{
	namespace nsActor
	{
		void EnemySituation::Clear()
		{
			/* すべての事実を初期化する。*/
			bSelfDead_ = false;
			bTargetDead_ = false;
			bInAggro_ = false;
			bVisible_ = false;
			bInAttack_ = false;
			bNeedKnockBack_ = false;
			bKnockBackDone_ = false;
		}


		uint32_t EnemySituation::MakeFactMask() const
		{
			/* bool の事実を遷移照合用のビットに落とす。*/
			uint32_t uMask = enEnemyFact_None;

			if (bSelfDead_)
				uMask |= enEnemyFact_SelfDead;

			if (bTargetDead_)
				uMask |= enEnemyFact_TargetDead;

			if (bInAggro_)
				uMask |= enEnemyFact_InAggro;
			else
				uMask |= enEnemyFact_NotInAggro;

			if (bVisible_)
				uMask |= enEnemyFact_Visible;

			if (bInAttack_)
				uMask |= enEnemyFact_InAttack;
			
			else
				uMask |= enEnemyFact_NotInAttack;

			if (bNeedKnockBack_)
				uMask |= enEnemyFact_NeedKnockBack;

			if (bKnockBackDone_)
				uMask |= enEnemyFact_KnockBackDone;

			return uMask;
		}


		bool EnemySituation::Matches(uint32_t uNeed) const
		{
			/* 現在の事実が、要求ビットをすべて含んでいるか。*/
			const uint32_t uHave = MakeFactMask();
			return (uHave & uNeed) == uNeed;
		}
	}
}