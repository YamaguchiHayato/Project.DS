#include "stdafx.h"
#include "EffectListener.h"

namespace
{
	/*
	 * 通知の種別ごとの再生設定表。
	 * エフェクトが増えても、この表に1行足すだけで対応できる。
	 * 表示倍率は、素材の大きさをこのゲームの縮尺(プレイヤー身長≒175)へ合わせるための値。
	 */
	const std::unordered_map<uint8_t, nsApp::nsEffect::EffectPlaySetting> EFFECT_SETTING_TABLE =
	{
		/* 発射: 銃口に閃光を出し、射線の方向へ向ける。*/
		{ static_cast<uint8_t>(nsApp::nsEvent::EnGameEvent::WeaponFired),
			{ nsApp::nsEffect::EnEffectID::MuzzleFlash, 8.0f, 0.2f, true } },

		/* 命中: 当たった位置に、飛んできた方向へ向けて出す。*/
		{ static_cast<uint8_t>(nsApp::nsEvent::EnGameEvent::BulletHit),
			{ nsApp::nsEffect::EnEffectID::Hit, 10.0f, 0.5f, true } },

		/* 爆発: 爆心地に出す。*/
		{ static_cast<uint8_t>(nsApp::nsEvent::EnGameEvent::GrenadeExploded),
			{ nsApp::nsEffect::EnEffectID::Explosion, 25.0f, 2.0f, false } },

		/* 回復: プレイヤーの位置に出す。*/
		{ static_cast<uint8_t>(nsApp::nsEvent::EnGameEvent::PlayerHealed),
			{ nsApp::nsEffect::EnEffectID::Heal, 15.0f, 1.5f, false } },
	};

	/**
	 * @brief 方向ベクトルから、その向きを向く回転を作る。
	 * @param vDirection 向ける方向(ゼロベクトルなら回転なし)。
	 * @return 生成した回転。
	 */
	Quaternion MakeRotationFromDirection(const Vector3& vDirection)
	{
		/* 方向が指定されていなければ回転させない。*/
		if (vDirection.Length() <= 0.0001f)
			return Quaternion::Identity;

		/* 長さを揃えて方向だけを取り出す。*/
		Vector3 vDir = vDirection;
		vDir.Normalize();

		/* 水平のヨーを求める。*/
		Quaternion qRotation;
		qRotation.SetRotationY(atan2f(vDir.x, vDir.z));

		/* 逆三角関数が扱える範囲へ収める。*/
		float fSin = vDir.y;
		if (fSin > 1.0f)
			fSin = 1.0f;
		else if (fSin < -1.0f)
			fSin = -1.0f;

		/* 上下のピッチをローカル軸で後乗せする。*/
		qRotation.AddRotationX(-asinf(fSin));

		return qRotation;
	}
}

namespace nsApp
{
	namespace nsEffect
	{
		void EffectListener::OnGameEvent(const nsEvent::GameEvent& stEvent)
		{
			/* 再生先が無ければ何もしない。*/
			if (pEffectList_ == nullptr)
				return;

			/* 表に載っていない通知では何も再生しない。*/
			const auto iterator = EFFECT_SETTING_TABLE.find(static_cast<uint8_t>(stEvent.enType_));
			if (iterator == EFFECT_SETTING_TABLE.end())
				return;

			/* 設定に従ってエフェクトを再生する。*/
			const EffectPlaySetting& stSetting = iterator->second;
			const Quaternion qRotation = stSetting.bUseDirection_ ? MakeRotationFromDirection(stEvent.vDirection_) : Quaternion::Identity;
			pEffectList_->PlayEffect(stSetting.enID_, stEvent.vPosition_, qRotation, Vector3::One * stSetting.fScale_, stSetting.fLifeTime_);
		}
	}
}
