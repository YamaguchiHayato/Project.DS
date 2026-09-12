#include "stdafx.h"
#include "PipeBomb.h"
#include "Player.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Combat/HitBoxSet.h"
#include "Src/Event/EventBus.h"
#include "Src/System/GamePause.h"

namespace
{
	const char* sPipeBombModelPath_ = "Assets/modelData/preset/VolumePointLight.tkm";	//! 仮モデル(発光球)。

	const float kThrowSpeed = 900.0f;		//! 前方への初速。
	const float kThrowUp = 300.0f;			//! 上方向への初速(山なりに投げる)。
	const float kGravity = 980.0f;			//! 重力加速度。
	const float kGroundHeight = 0.0f;		//! 着地する高さ(床)。
	const float kBaseScale = 12.0f;			//! 表示サイズ。
	const float kBlinkScale = 6.0f;			//! 明滅で大きくなる量。
	const float kBlinkSpeedMin = 6.0f;		//! 投げた直後の明滅の速さ。
	const float kBlinkSpeedMax = 30.0f;		//! 起爆直前の明滅の速さ。
	const float kFuseTime = 6.0f;			//! 着地してから爆発するまでの時間(秒)。本家と同じくらい。
	const float kAttractRadius = 400.0f;	//! この距離の雑魚を引き寄せる。敵のアグロ距離より少し広い程度にしている。
	const float kAttractInterval = 0.2f;	//! 引き寄せを見直す間隔(秒)。毎フレーム全敵を見ないため。
	const float kExplodeRadius = 350.0f;	//! 爆発の有効半径。
	const int kExplodeDamage = 100;			//! 爆発の威力(範囲内の敵へ)。雑魚は確実に倒せる量。
	const int kDecoyHP = 100000;			//! おとりとして殴られても尽きないHP。
	const float kExplodeEffectHeight = 50.0f;	//! 爆発エフェクトを出す高さ。
}

namespace nsApp
{
	namespace nsItem
	{
		bool PipeBomb::Start()
		{
			/* 敵に狙われても「倒された」ことにならないよう、HPは大きく持つ。*/
			stCharacterStatus_.stHp_.iMaxHP_ = kDecoyHP;
			stCharacterStatus_.stHp_.iCurrentHP_ = kDecoyHP;

			/* 仮モデルを置く。*/
			stModelRender_.Init(sPipeBombModelPath_, nullptr, 0, enModelUpAxisY);
			stModelRender_.SetScale(Vector3(kBaseScale, kBaseScale, kBaseScale));
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();

			fFuse_ = kFuseTime;
			return true;
		}


		void PipeBomb::Update()
		{
			/* ICharacter(Actor)の共通更新処理。*/
			ICharacter::Update();

			/* ポーズ中は止める。*/
			if (nsSystem::IsGamePaused())
				return;

			const float fDeltaTime = g_gameTime->GetFrameDeltaTime();

			UpdateFlight(fDeltaTime);
			UpdateAttract(fDeltaTime);
			UpdateBlink(fDeltaTime);

			/* 信管は投げた瞬間から進む。切れたら爆発して消える。*/
			fFuse_ -= fDeltaTime;
			if (fFuse_ <= 0.0f)
			{
				Explode();
				DeleteGO(this);
				return;
			}

			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();
		}


		void PipeBomb::UpdateFlight(float fDeltaTime)
		{
			/* 着地したら転がらずその場に止まる。*/
			if (bIsLanded_)
				return;

			/* 簡易物理: 重力で放物線を描く。*/
			vVelocity_.y -= kGravity * fDeltaTime;
			vPosition_ += vVelocity_ * fDeltaTime;

			/* 床に着いたら止める。*/
			if (vPosition_.y <= kGroundHeight)
			{
				vPosition_.y = kGroundHeight;
				vVelocity_ = Vector3::Zero;
				bIsLanded_ = true;
			}
		}


		void PipeBomb::UpdateAttract(float fDeltaTime)
		{
			/* 毎フレーム全敵を見ないよう、一定間隔で見直す。*/
			fAttractTimer_ -= fDeltaTime;
			if (fAttractTimer_ > 0.0f)
				return;

			fAttractTimer_ = kAttractInterval;

			/* 近くの雑魚の追跡対象を自分に向ける。敵は自分の位置へ向かって来て殴る(HPは尽きない)。*/
			for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
			{
				if (pEnemy == nullptr)
					continue;

				Vector3 vDiff = pEnemy->GetPosition() - vPosition_;
				vDiff.y = 0.0f;
				if (vDiff.Length() > kAttractRadius)
					continue;

				pEnemy->SetTarget(this);
			}
		}


		void PipeBomb::UpdateBlink(float fDeltaTime)
		{
			/* 残り時間が短いほど速く明滅させ、もう爆発すると分かるようにする。*/
			const float fRemainRate = (kFuseTime > 0.0f) ? (fFuse_ / kFuseTime) : 0.0f;
			const float fBlinkSpeed = kBlinkSpeedMax + (kBlinkSpeedMin - kBlinkSpeedMax) * fRemainRate;
			fBlinkPhase_ += fBlinkSpeed * fDeltaTime;

			/* sin波(-1〜1)を0〜1へ均して大きさに足す。*/
			const float fBlink = sinf(fBlinkPhase_) * 0.5f + 0.5f;
			const float fScale = kBaseScale + kBlinkScale * fBlink;
			stModelRender_.SetScale(Vector3(fScale, fScale, fScale));
		}


		void PipeBomb::RestoreEnemyTargets()
		{
			/* 戻す先のプレイヤー。シーンの片付け中などで居なければ何もできない。*/
			nsActor::Player* pPlayer = FindGO<nsActor::Player>("player");
			if (pPlayer == nullptr)
				return;

			/*
			 * 自分を狙っていた敵だけを選べないので、生きている雑魚を全てプレイヤーへ向け直す。
			 * 追跡対象はプレイヤーしか居ないので、もともとプレイヤーを狙っていた敵は何も変わらない。
			 */
			for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
			{
				if (pEnemy != nullptr)
					pEnemy->SetTarget(pPlayer);
			}
		}


		void PipeBomb::Explode()
		{
			nsEvent::EventBus* pBus = FindGO<nsEvent::EventBus>("eventBus");

			/* 爆発エフェクト(大きめの閃光)。*/
			if (pBus != nullptr)
			{
				nsEvent::GameEvent stEvent;
				stEvent.enType_ = nsEvent::EnGameEvent::BombExploded;
				stEvent.vPosition_ = vPosition_;
				stEvent.vPosition_.y += kExplodeEffectHeight;
				pBus->Publish(stEvent);
			}

			/* 撃破エフェクトを出す高さ(胸のあたり)。*/
			const float fKillHeight = nsCombat::HitBoxSet::GetShared(CharacterModelType::Infected).GetHeight() * 0.5f;

			/* 有効半径内の敵にまとめてダメージを与える。*/
			for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
			{
				if (pEnemy == nullptr)
					continue;

				Vector3 vDiff = pEnemy->GetPosition() - vPosition_;
				if (vDiff.Length() > kExplodeRadius)
					continue;

				pEnemy->ApplyDamage(kExplodeDamage);

				/* 被弾の演出。*/
				Vector3 vHitPos = pEnemy->GetPosition();
				vHitPos.y += fKillHeight;
				if (pBus != nullptr)
				{
					nsEvent::GameEvent stHitEvent;
					stHitEvent.enType_ = nsEvent::EnGameEvent::BulletHit;
					stHitEvent.vPosition_ = vHitPos;
					stHitEvent.iParam_ = kExplodeDamage;
					pBus->Publish(stHitEvent);
				}

				/* 倒したら撃破の通知(戦績に数える)を出して退場させる。*/
				if (!pEnemy->IsDead())
					continue;

				if (pBus != nullptr)
				{
					nsEvent::GameEvent stKillEvent;
					stKillEvent.enType_ = nsEvent::EnGameEvent::EnemyKilled;
					stKillEvent.vPosition_ = vHitPos;
					pBus->Publish(stKillEvent);
				}

				DeleteGO(pEnemy);
			}

			/* 自分が消える前に、生き残った敵の追跡対象をプレイヤーへ戻す。*/
			RestoreEnemyTargets();
		}


		void PipeBomb::Render(RenderContext& rc)
		{
			stModelRender_.Draw(rc);
		}


		void PipeBomb::Setup(const Vector3& vPos, const Vector3& vDir)
		{
			vPosition_ = vPos;

			Vector3 vNormalized = vDir;
			vNormalized.Normalize();

			/* 前方初速＋上方向初速で山なりに飛ばす。*/
			vVelocity_ = vNormalized * kThrowSpeed;
			vVelocity_.y += kThrowUp;
		}
	}
}
