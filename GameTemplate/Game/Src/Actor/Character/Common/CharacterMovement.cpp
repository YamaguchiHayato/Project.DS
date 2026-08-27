#include "stdafx.h"
#include "CharacterMovement.h"

namespace
{
	const float kGravity_ = 980.0f; //! 重力加速度。Grenadeと同じ単位。
}

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
		/* 初期化されていなければ位置を返すだけ。*/
		if (!bIsInited_)
			return vPosition_;

		/* 移動速度をコピーする。*/
		Vector3 vMoveSpeed = vSpeed;

		/* 重力を使う場合は落下速度を加える。*/
		if (bUseGravity_)
		{
			/* Sample07系: 接地中は落下速度リセット、空中だけ重力を積む。*/
			if (stCharaCon_.IsOnGround())
				fFallSpeed_ = 0.0f;
			else
				fFallSpeed_ -= kGravity_ * fDeltaTime;

			/* 落下速度を移動速度に加える。*/
			vMoveSpeed.y = fFallSpeed_;
		}

		/* キャラコンを実行して移動する。*/
		vPosition_ = stCharaCon_.Execute(vMoveSpeed, fDeltaTime);

		/* 着地したら落下速度を捨てる。*/
		if (bUseGravity_ && stCharaCon_.IsOnGround())
			fFallSpeed_ = 0.0f;

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