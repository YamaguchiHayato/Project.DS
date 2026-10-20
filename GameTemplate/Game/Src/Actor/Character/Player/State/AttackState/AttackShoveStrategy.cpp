#include "stdafx.h"
#include "AttackShoveStrategy.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"

namespace
{
	const float ENEMY_CENTER_HEIGHT_FOR_SHOVE = 85.0f;
	const int SHOVE_DAMAGE = 10;
}

namespace nsApp
{
	namespace nsActor
	{
		void AttackShoveStrategy::Execute(Player* pPlayer, float fDeltaTime)
		{
			/* プレイヤーがいなければ何もしない。*/
			if (pPlayer == nullptr)
				return;

			/* ダウン中は近接しない（Alive のときだけ）。*/
			if (pPlayer->GetLifeState() != EnLifeState::Alive)
				return;

			/* クールダウンを進める。*/
			if (pPlayer->GetShoveCooldown() > 0.0f)
				pPlayer->AddShoveCooldown(-fDeltaTime);

			/* 押さずにいた時間で疲労が1回ぶんずつ抜けていく。*/
			pPlayer->AddShoveRestTimer(fDeltaTime);
			while (pPlayer->GetShoveFatigue() > 0 && pPlayer->GetShoveRestTimer() >= pPlayer->GetPlayerStatus().fShoveFatigueRecoverTime_)
			{
				pPlayer->AddShoveStack(-1);
				pPlayer->AddShoveRestTimer(-pPlayer->GetPlayerStatus().fShoveFatigueRecoverTime_);
			}

			/* 突き出した銃を戻す。*/
			workMotionRate_ = fDeltaTime * pPlayer->GetPlayerStatus().fShoveMotionRecoverRate_;
			if (workMotionRate_ > 1.0f)
				workMotionRate_ = 1.0f;

			/* 突き出しの疲労度を減らす。*/
			pPlayer->AddShoveMotion(-pPlayer->GetShoveMotion() * workMotionRate_);

			/* 入力が無い、またはクールダウン中なら何もしない。*/
			if (!pPlayer->GetIntent().bShoveTrigger_ || pPlayer->GetShoveCooldown() > 0.0f)
				return;

			/* メディキット使用中は押せない。*/
			if (pPlayer->IsHealing())
				return;

			/* しゃがみ中は押せない。*/
			pPlayer->AddShoveStack(1);
			pPlayer->SetShoveRestTimer(0.0f);
			workIsFatigued_ = !pPlayer->IsAdrenalineActive() && (pPlayer->GetShoveFatigue() >= pPlayer->GetPlayerStatus().iShoveFatigueCount_);
			pPlayer->SetShoveCooldown(workIsFatigued_ ? pPlayer->GetPlayerStatus().fShoveFatigueCooldownTime_ : pPlayer->GetPlayerStatus().fShoveCooldownTime_);

			/* 突き出しの疲労度を増やす。*/
			pPlayer->SetShoveMotion(1.0f);
			pPlayer->GetWeaponInventory().CancelReload();

			/* プレイヤーの向きベクトルを計算する。*/
			workAimDir_ = { sinf(pPlayer->GetCameraYaw()), 0.0f, cosf(pPlayer->GetCameraYaw()) };

			/* 近くの敵を押す。*/
			for (CommonEnemy* pEnemy : FindGOs<CommonEnemy>("commonEnemy"))
			{
				/* 敵がいなければスキップ。*/
				if (pEnemy == nullptr)
					continue;

				/* 敵が死んでいればスキップ。*/
				workToEnemy_ = pEnemy->GetPosition() - pPlayer->GetPosition();
				workToEnemy_.y = 0.0f;
				workDist_ = workToEnemy_.Length();

				/* 範囲外の敵はスキップ。*/
				if (workDist_ <= 0.0001f || workDist_ > pPlayer->GetPlayerStatus().fShoveRange_)
					continue;

				/* 敵の方向ベクトルを正規化する。*/
				workDir_ = workToEnemy_;
				workDir_.Normalize();

				/* プレイヤーの正面方向と敵の方向の角度が広すぎる場合はスキップ。*/
				if (workDir_.Dot(workAimDir_) < pPlayer->GetPlayerStatus().fShoveFrontDot_)
					continue;

				/* 敵を押す。*/
				pEnemy->SetPosition(pEnemy->GetPosition() + workDir_ * pPlayer->GetPlayerStatus().fShovePush_);
				pEnemy->ApplyDamage(SHOVE_DAMAGE);

				/* 敵の中心位置を計算して、イベントを発行する。*/
				workShoveHitPos_ = pEnemy->GetPosition();
				workShoveHitPos_.y += ENEMY_CENTER_HEIGHT_FOR_SHOVE;
				pPlayer->PublishGameEvent(nsEvent::EnGameEvent::BulletHit, workShoveHitPos_, workDir_, SHOVE_DAMAGE);

				/* 敵が死んでいれば、敵撃破イベントを発行して敵を削除する。*/
				if (pEnemy->IsDead())
				{
					pPlayer->PublishGameEvent(nsEvent::EnGameEvent::EnemyKilled, pEnemy->GetPosition());
					DeleteGO(pEnemy);
				}
			}

			/* デバッグ用。*/
			OutputDebugStringA("[Player] Shove!\n");
		}
	}
}