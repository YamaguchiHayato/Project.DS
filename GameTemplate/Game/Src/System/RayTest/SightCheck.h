#pragma once

#include "Src/System/RayTest/RaycastQuery.h"

namespace nsApp
{
	namespace nsSystem
	{
		/**
		 * @file   SightCheck.h
		 * @brief  視線が通っているかの判定。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/21
		 */
		class SightCheck
		{
		public:
			SightCheck() = default;
			virtual ~SightCheck() = default;


		public:
			/**
			 * @brief 始点から終点まで遮蔽されていないか。
			 * @param vFrom 始点。
			 * @param vTo 終点。
			 * @return 遮蔽されていなければ true。
			 */
			bool HasClearSight(const Vector3& vFrom, const Vector3& vTo) const;

			/**
			 * @brief 目の高さを取得する。
			 */
			inline float GetEyeHeight() const
			{
				return fEyeHeight_;
			}

			/**
			 * @brief 目の高さを設定する。
			 */
			inline void SetEyeHeight(float fHeight)
			{
				fEyeHeight_ = fHeight;
			}


		private:
			/**
			 * @brief ヒットが終点より手前か。
			 */
			bool IsHitBeforeTarget(const Vector3& vFrom, const Vector3& vTo, const Vector3& vHitPos) const;


		private:
			RaycastQuery stRaycastQuery_; //! レイ問い合わせ。
			float fEyeHeight_ = 50.0f; //! 目の高さオフセット。
			float fHitSlack_ = 10.0f; //! 終点判定の余裕。
		};
	}
}