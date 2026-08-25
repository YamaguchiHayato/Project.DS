#include "stdafx.h"
#include "SightCheck.h"

namespace nsApp
{
	namespace nsSystem
	{
		bool SightCheck::HasClearSight(const Vector3& vFrom, const Vector3& vTo) const
		{
			/* 始点から終点までレイを飛ばす。*/
			const RaycastHit stHit = stRaycastQuery_.Trace(vFrom, vTo);
			if (!stHit.bHit_)
				return true;

			/* ヒット位置が終点より手前かを判定する。*/
			return !IsHitBeforeTarget(vFrom, vTo, stHit.vHitPos_);
		}


		bool SightCheck::IsHitBeforeTarget(const Vector3& vFrom, const Vector3& vTo, const Vector3& vHitPos) const
		{
			/* 始点から終点までの距離と、始点からヒット位置までの距離を比較する。*/
			const float fDistToTarget = (vTo - vFrom).Length();
			const float fDistToHit = (vHitPos - vFrom).Length();

			/* ヒット位置が終点より手前ならtrueを返す。*/
			return fDistToHit < fDistToTarget - fHitSlack_;
		}
	}
}