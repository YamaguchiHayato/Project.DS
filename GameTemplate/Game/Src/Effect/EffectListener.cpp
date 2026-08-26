#include "stdafx.h"
#include "EffectListener.h"
#include "EffectList.h"

namespace
{
	const float	fMuzzleLifeTime_ = 0.2f;	//! 銃口の閃光の表示時間(秒)。
	const float	fHitLifeTime_ = 0.5f;		//! 命中エフェクトの表示時間(秒)。
	const float	fExplosionLifeTime_ = 2.0f;	//! 爆発エフェクトの表示時間(秒)。
	const float	fHealLifeTime_ = 1.5f;		//! 回復エフェクトの表示時間(秒)。

	/*
	 * 素材ごとの表示倍率。エフェクト素材は別プロジェクト向けに作られているため、
	 * このゲームの縮尺(プレイヤー身長≒175)に合わせてここで倍率を掛ける。
	 * 大きすぎる/小さすぎる場合はこの値を調整する。
	 */
	const float	fMuzzleScale_ = 8.0f;		//! 銃口の閃光の表示倍率。
	const float	fHitScale_ = 10.0f;			//! 命中エフェクトの表示倍率。
	const float	fExplosionScale_ = 25.0f;	//! 爆発エフェクトの表示倍率。
	const float	fHealScale_ = 15.0f;		//! 回復エフェクトの表示倍率。

	/**
	 * @brief 方向ベクトルから、その向きを向く回転を作る。
	 * @param vDirection 向ける方向(ゼロベクトルなら回転なし)。
	 * @return 生成した回転。
	 */
	Quaternion MakeRotationFromDirection(const Vector3& vDirection)
	{
		/* 方向が指定されていなければ回転させない。*/
		if (vDirection.x == 0.0f && vDirection.y == 0.0f && vDirection.z == 0.0f)
			return Quaternion::Identity;

		Vector3 vDir = vDirection;
		vDir.Normalize();

		/* 水平のヨーを求め、上下のピッチをローカル軸で後乗せする。*/
		Quaternion qRotation;
		qRotation.SetRotationY(atan2f(vDir.x, vDir.z));

		float fSin = vDir.y;
		if (fSin > 1.0f) fSin = 1.0f;
		else if (fSin < -1.0f) fSin = -1.0f;
		qRotation.AddRotationX(-asinf(fSin));

		return qRotation;
	}
}

namespace nsApp
{
	namespace nsEffect
	{
		void EffectListener::Initialize(EffectList* pEffectList)
		{
			pEffectList_ = pEffectList;
		}


		void EffectListener::OnGameEvent(const nsEvent::GameEvent& stEvent)
		{
			/* 再生先が無ければ何もしない。*/
			if (pEffectList_ == nullptr)
				return;

			/* 出来事の種類に応じて再生するエフェクトを決める。*/
			switch (stEvent.enType_)
			{
			case nsEvent::EnGameEvent::WeaponFired:
				/* 発射: 銃口に閃光を出し、射線の方向へ向ける。*/
				pEffectList_->PlayEffect(
					EnEffectID::MuzzleFlash,
					stEvent.vPosition_,
					MakeRotationFromDirection(stEvent.vDirection_),
					Vector3::One * fMuzzleScale_,
					fMuzzleLifeTime_);
				break;

			case nsEvent::EnGameEvent::BulletHit:
				/* 命中: 当たった位置に出す。*/
				pEffectList_->PlayEffect(
					EnEffectID::Hit,
					stEvent.vPosition_,
					MakeRotationFromDirection(stEvent.vDirection_),
					Vector3::One * fHitScale_,
					fHitLifeTime_);
				break;

			case nsEvent::EnGameEvent::GrenadeExploded:
				/* 爆発: 爆心地に出す。*/
				pEffectList_->PlayEffect(
					EnEffectID::Explosion,
					stEvent.vPosition_,
					Quaternion::Identity,
					Vector3::One * fExplosionScale_,
					fExplosionLifeTime_);
				break;

			case nsEvent::EnGameEvent::PlayerHealed:
				/* 回復: プレイヤーの位置に出す。*/
				pEffectList_->PlayEffect(
					EnEffectID::Heal,
					stEvent.vPosition_,
					Quaternion::Identity,
					Vector3::One * fHealScale_,
					fHealLifeTime_);
				break;

			default:
				/* それ以外の通知では何も再生しない。*/
				break;
			}
		}
	}
}
