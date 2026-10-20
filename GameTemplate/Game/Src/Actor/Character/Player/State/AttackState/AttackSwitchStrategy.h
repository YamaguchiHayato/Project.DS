#pragma once
#include "IAttackStrategy.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   AttackSwitchStrategy.h
		 * @brief  武器切り替えの具象戦略。
		 * @details 数字キー／ホイールでの持ち替え。ダウン中は行わない。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/20
		 */
		class AttackSwitchStrategy : public IAttackStrategy
		{
		public:
			/**
			 * @brief この機能を1フレ分実行する。
			 * @param pPlayer    実行対象。
			 * @param fDeltaTime 経過時間(秒)。使わない戦略は無視してよい。
			 */
			void Execute(Player* pPlayer, float fDeltaTime) override;
		};
	}
}