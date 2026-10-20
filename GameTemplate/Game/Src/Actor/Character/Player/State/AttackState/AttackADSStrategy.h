#pragma once
#include "IAttackStrategy.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   AttackAdsStrategy.h
		 * @brief  覗き込み・銃下げ・連射拡散の回収（旧 UpdateAds）。
		 * @details 移設元: PlayerUpdate::UpdateAds / ApproachAds / ApproachLower / RecoverSpread。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/20
		 */
		class AttackAdsStrategy : public IAttackStrategy
		{
		public:
			/**
			 * @brief 覗き込みまわりを1フレ進める。
			 */
			void Execute(Player* pPlayer, float fDeltaTime) override;


		private:
			float workBlendRate_ = 0.0f; //! 寄せ速度。
		};
	}
}