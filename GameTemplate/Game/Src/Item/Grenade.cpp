#include "stdafx.h"
#include "Grenade.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Event/EventBus.h"
#include "Src/System/GamePause.h"

namespace
{
	const char* sGrenadeModelPath_ = "Assets/modelData/preset/VolumePointLight.tkm";	//! 仮モデル(発光球)。
	const float kThrowSpeed = 900.0f;		//! 前方への初速。
	const float kThrowUp = 300.0f;			//! 上方向への初速(山なりに投げる)。
	const float kGravity = 980.0f;			//! 重力加速度。
	const float kGrenadeScale = 12.0f;		//! 表示サイズ。
	const float kExplodeRadius = 300.0f;	//! 爆発の有効半径。
	const int kExplodeDamage = 100;		//! 爆発の威力(範囲内の敵へ)。
	const float kEnemyCenterHeight = 85.0f;	//! 敵の体の中心高さ(エフェクト用)。
}

namespace nsApp
{
	namespace nsItem
	{
		bool Grenade::Start()
		{
			/* 仮モデルを置く。*/
			stModel_.Init(sGrenadeModelPath_, nullptr, 0, enModelUpAxisY);
			stModel_.SetScale(Vector3(kGrenadeScale, kGrenadeScale, kGrenadeScale));
			stModel_.SetPosition(vPos_);
			stModel_.Update();
			return true;
		}


		void Grenade::Update()
		{
			/* ポーズ中は止める。*/
			if (nsSystem::IsGamePaused())
				return;

			const float fDeltaTime = g_gameTime->GetFrameDeltaTime();

			/* 簡易物理: 重力で放物線を描く。*/
			fFuse_ -= fDeltaTime;
			vVel_.y -= kGravity * fDeltaTime;
			vPos_ += vVel_ * fDeltaTime;

			/* 着地(地面)または信管切れで爆発して消える。*/
			if (vPos_.y <= 0.0f || fFuse_ <= 0.0f)
			{
				vPos_.y = 0.0f;
				Explode();
				DeleteGO(this);
				return;
			}

			stModel_.SetPosition(vPos_);
			stModel_.Update();
		}


		void Grenade::Render(RenderContext& rc)
		{
			stModel_.Draw(rc);
		}


		void Grenade::Setup(const Vector3& vPos, const Vector3& vDir)
		{
			vPos_ = vPos;

			Vector3 vNormalized = vDir;
			vNormalized.Normalize();

			/* 前方初速＋上方向初速で山なりに飛ばす。*/
			vVel_ = vNormalized * kThrowSpeed;
			vVel_.y += kThrowUp;
		}


		void Grenade::Explode()
		{
			/* 爆発エフェクト(大きめの閃光)。*/
			Vector3 vFxPos = vPos_;
			vFxPos.y += 50.0f;
			nsEvent::EventBus* pBus = FindGO<nsEvent::EventBus>("eventBus");
			if (pBus != nullptr)
			{
				nsEvent::GameEvent stEvent;
				stEvent.enType_ = nsEvent::EnGameEvent::GrenadeExploded;
				stEvent.vPosition_ = vFxPos;
				pBus->Publish(stEvent);
			}

			/* 有効半径内の敵にまとめてダメージを与える。*/
			for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
			{
				if (pEnemy == nullptr)
					continue;

				Vector3 vDiff = pEnemy->GetPosition() - vPos_;
				if (vDiff.Length() > kExplodeRadius)
					continue;

				pEnemy->ApplyDamage(kExplodeDamage);

				/* 被弾エフェクト。*/
				Vector3 vHitPos = pEnemy->GetPosition();
				vHitPos.y += kEnemyCenterHeight;
				if (pBus != nullptr)
				{
					nsEvent::GameEvent stHitEvent;
					stHitEvent.enType_ = nsEvent::EnGameEvent::BulletHit;
					stHitEvent.vPosition_ = vHitPos;
					pBus->Publish(stHitEvent);
				}

				/* 倒したら退場させる。*/
				if (pEnemy->IsDead())
					DeleteGO(pEnemy);
			}
		}
	}
}
