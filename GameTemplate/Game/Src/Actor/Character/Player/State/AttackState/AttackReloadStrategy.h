#pragma once
#include "IAttackStrategy.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   AttackReloadStrategy.h
		 * @brief  リロードの具象戦略。
		 * @details Context（PlayerAttackState）から Execute で呼ばれる。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/20
		 */
		class AttackReloadStrategy : public IAttackStrategy
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