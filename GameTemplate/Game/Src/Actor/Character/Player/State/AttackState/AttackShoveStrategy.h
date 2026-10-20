#pragma once
#include "IAttackStrategy.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   AttackShoveStrategy.h
		 * @brief  突き飛ばし（近接）の具象戦略。
		 * @details 移設元: PlayerCombat::UpdateShove。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/20
		 */
		class AttackShoveStrategy : public IAttackStrategy
		{
		public:
			/**
			 * @brief クールダウン・疲労・押し返しを1フレ進める。
			 * @param pPlayer    実行対象。
			 * @param fDeltaTime 経過時間(秒)。
			 */
			void Execute(Player* pPlayer, float fDeltaTime) override;


		private:
			float workMotionRate_ = 0.0f;
			bool workIsFatigued_ = false;
			Vector3 workAimDir_ = Vector3::Zero;
			Vector3 workToEnemy_ = Vector3::Zero;
			float workDist_ = 0.0f;
			Vector3 workDir_ = Vector3::Zero;
			Vector3 workShoveHitPos_ = Vector3::Zero;
		};
	}
}