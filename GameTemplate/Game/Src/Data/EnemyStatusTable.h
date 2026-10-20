#pragma once
#include "EnemyStatus.h"
#include <unordered_map>

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @brief enemyStatus.tsv を種類ごとに保持する。
		 */
		class EnemyStatusTable
		{
		public:
			/**
			 * @brief 種類に対応する行を返す。初回だけ TSV を読む。
			 */
			static const EnemyStatus& Get(nsActor::EnEnemyType enType);

			/**
			 * @brief TSV を読み直す。
			 */
			static void Load();
		};
	}
}