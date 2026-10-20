#pragma once
#include "Src/Actor/Character/Enemy/EnEnemyType.h"

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @brief ゾンビ1種類分のステータス。enemyStatus.tsv の1行。
		 */
		struct EnemyStatus
		{
			nsActor::EnEnemyType enType_ = nsActor::EnEnemyType::None;
			int iMaxHp_ = 1;
			float fAggroRange_ = 250.0f;
			float fChaseSpeed_ = 60.0f;
			float fAttackRange_ = 80.0f;
			float fExplodeRange_ = 0.0f;
			int iExplodeDamage_ = 0;

			/**
			 * @brief TSVの1行からステータスを作る。
			 */
			static bool Create(const nsK2EngineLow::nsSystem::nsTSVFile::TSVTable& table, int row, EnemyStatus& outStatus);
		};
	}
}