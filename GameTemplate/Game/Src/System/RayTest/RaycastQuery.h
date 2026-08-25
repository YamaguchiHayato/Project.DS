#pragma once

namespace nsApp
{
	namespace nsSystem
	{
		/**
		 * @struct RaycastHit
		 * @brief レイキャストの結果。
		 */
		struct RaycastHit
		{
			bool bHit_ = false;					//! ヒットしたか。
			Vector3 vHitPos_ = Vector3::Zero;	//! ヒット位置。
			float fDistance_ = 0.0f;			//! 始点からヒットまでの距離。
		};

		/**
		 * @file   RaycastQuery.h
		 * @brief  物理ワールドへのレイ問い合わせ。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/21
		 */
		class RaycastQuery
		{
		public:
			/* コンストラクタ */
			RaycastQuery() = default;
			virtual ~RaycastQuery() = default;
			
			/**
			 * @brief 始点から終点へレイを飛ばす。
			 * @param vFrom 始点。
			 * @param vTo 終点。
			 * @return ヒット結果。
			 */
			RaycastHit Trace(const Vector3& vFrom, const Vector3& vTo) const;
		};
	}
}