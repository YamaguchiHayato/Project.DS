#pragma once

namespace nsApp
{
	namespace nsActor
	{
		class Player;

		/**
		 * @file  IAttackStrategy.h
		 * @brief Attack 内の機能戦略（射撃・リロード等へ分割する土台）。
		 * @details PlayerAttackState 内で使う。将来、射撃・リロード・ADS などの挙動を分割するためのインターフェース。
		 * @autor Yamaguchi Hayato
		 * date 2026/10/20: 新規作成日。
		 */
		class IAttackStrategy
		{
		public:
			/* デストラクタ。*/
			virtual ~IAttackStrategy() = default;

			/**
			 * @brief この機能を1フレ分実行する。
			 * @param pPlayer    実行対象。
			 * @param fDeltaTime 経過時間(秒)。使わない戦略は無視してよい。
			 */
			virtual void Execute(Player* pPlayer, float fDeltaTime) = 0;
		};
	}
}