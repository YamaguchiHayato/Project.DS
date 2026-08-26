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


	void CharacterMovement::SetPosition(const Vector3& vPosition)
	{
		vPosition_ = vPosition;

		/* 初期化済みならキャラコン側の座標も合わせる。*/
		if (bIsInited_)
			stCharaCon_.SetPosition(vPosition_);
	}
}