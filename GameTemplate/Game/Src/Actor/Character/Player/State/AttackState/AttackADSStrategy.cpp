#include "stdafx.h"
#include "AttackAdsStrategy.h"
#include "Src/Actor/Character/Player/Player.h"

namespace
{
	const float ADS_TRANSITION_RATE = 12.0f;	//! 覗き込みの切り替わる速さ。
	const float SPREAD_RECOVER_RATE = 4.0f;		//! 連射拡散が収まる速さ。
	const float SPRINT_LOWER_RATE = 8.0f;		//! 銃を下げる/戻す速さ。
}


namespace nsApp
{
	namespace nsActor
	{
		void AttackAdsStrategy::Execute(Player* pPlayer, float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer == nullptr)
				return;

			/* プレイヤーの意図を取得する。*/
			const PlayerIntent& stIntent = pPlayer->GetIntent();

			/* --- ApproachAds: 覗き込み度合いを目標へ寄せる --- */
			workBlendRate_ = fDeltaTime * ADS_TRANSITION_RATE;
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* 走っている間は構えられない。*/
			pPlayer->AddAdsRate((((stIntent.bAdsPress_ && !pPlayer->IsSprinting()) ? 1.0f : 0.0f) - pPlayer->GetAdsRate()) * workBlendRate_);

			/* --- ApproachLower: スプリント／回復中は銃を下げる --- */
			workBlendRate_ = fDeltaTime * SPRINT_LOWER_RATE;
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* スプリント中は銃を下げる。回復中も下げる。*/
			pPlayer->AddLowerRate(((((pPlayer->IsSprinting() && pPlayer->IsMoving()) || pPlayer->IsHealing()) ? 1.0f : 0.0f) - pPlayer->GetLowerRate()) * workBlendRate_);

			/* --- RecoverSpread: 連射拡散を時間で収める --- */
			workBlendRate_ = fDeltaTime * SPREAD_RECOVER_RATE;
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* 連射拡散を時間で収める。*/
			pPlayer->AddSpreadShot(-pPlayer->GetSpreadShot() * workBlendRate_);
		}
	}
}