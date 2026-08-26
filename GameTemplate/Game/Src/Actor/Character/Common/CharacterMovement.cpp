#include "stdafx.h"
#include "CharacterMovement.h"

namespace nsApp
{
	void CharacterMovement::Init(float fRadius, float fHeight, const Vector3& vPosition)
	{
		/* 位置を保持してからキャラコンを初期化する。*/
		vPosition_ = vPosition;
		stCharaCon_.Init(fRadius, fHeight, vPosition_);
		bIsInited_ = true;
	}


	Vector3 CharacterMovement::Execute(const Vector3& vSpeed, float fDeltaTime)
	{
		/* 未初期化なら現在位置をそのまま返す。*/
		if (!bIsInited_)
			return vPosition_;

		/* 速度を書き換えてよいよう、ローカルにコピーする。*/
		Vector3 vMoveSpeed = vSpeed;

		/* キャラコンで衝突解決した位置を受け取る。*/
		vPosition_ = stCharaCon_.Execute(vMoveSpeed, fDeltaTime);
		return vPosition_;
	}


	Vector3 CharacterMovement::MoveOnDirection(const Vector3& vDirection, float fMaxSpeed, float fDeltaTime)
	{
		/* 方向と最大速度から水平速度を作り、移動する。*/
		const Vector3 vSpeed = MakeHorizontalSpeed(vDirection, fMaxSpeed);
		return Execute(vSpeed, fDeltaTime);
	}


	Vector3 CharacterMovement::MoveToward(const Vector3& vTarget, float fMaxSpeed, float fDeltaTime)
	{
		/* 現在位置から目標への方向を作り、その方向へ移動する。*/
		Vector3 vDirection = vTarget - vPosition_;
		vDirection.y = 0.0f;
		return MoveOnDirection(vDirection, fMaxSpeed, fDeltaTime);
	}


	void CharacterMovement::SetPosition(const Vector3& vPosition)
	{
		vPosition_ = vPosition;

		/* 初期化済みならキャラコン側の座標も合わせる。*/
		if (bIsInited_)
			stCharaCon_.SetPosition(vPosition_);
	}


	Vector3 CharacterMovement::MakeHorizontalSpeed(const Vector3& vDirection, float fMaxSpeed) const
	{
		/* 高さは無視した方向を使う。*/
		Vector3 vHorizontal = vDirection;
		vHorizontal.y = 0.0f;

		/* 方向が無ければ停止する。*/
		if (vHorizontal.Length() <= 0.0001f)
			return Vector3::Zero;

		/* 正規化して最大速度を掛ける。*/
		vHorizontal.Normalize();
		return vHorizontal * fMaxSpeed;
	}
}