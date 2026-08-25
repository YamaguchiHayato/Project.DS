#include "stdafx.h"
#include "RaycastQuery.h"

namespace nsApp
{
	namespace nsSystem
	{
		RaycastHit RaycastQuery::Trace(const Vector3& vFrom, const Vector3& vTo) const
		{
			/* レイキャストを実施する。*/
			RaycastHit stHit;

			/* レイキャストを実施する。*/
			stHit.bHit_ = nsK2EngineLow::PhysicsWorld::GetInstance()->RayTest(vFrom, vTo, stHit.vHitPos_);

			/* ヒットした場合は距離を計算する。*/
			if (stHit.bHit_)
				stHit.fDistance_ = (stHit.vHitPos_ - vFrom).Length();

			/* 結果を返す。*/
			return stHit;
		}
	}
}