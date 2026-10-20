#pragma once
#include "Src/Actor/Character/Player/State/PlayerAttackState.h"

namespace nsApp
{
	namespace nsActor
	{
		class Player;

		/**
		 * @file   PlayerCombat.h
		 * @brief  銃・近接など Attack 系の実処理置き場。
		 * @details ADS / 射撃 / リロード / 近接の中身は後でここへ移し、
		 *          AttackState から呼ぶ。メイン／サブは Strategy で切り替える。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/20: 最終更新日。
		 */
		class PlayerCombat
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PlayerCombat() = default;
			virtual ~PlayerCombat() = default;


		public:
			/**
			 * @brief 計算に使う参照を結びつける。
			 * @details ステータスや武器へのポインタ等は移設時に決める。
			 */
			inline void Bind(Player* pPlayer)
			{
				pPlayer_ = pPlayer;
			}

			/**
			 * @brief 武器更新（射撃・リロード等）。
			 * @details 移設元: Player::UpdateWeapon。後で Attack 内サブステートへ分割する。
			 */
			void UpdateWeapon(float fDeltaTime)
			{
				PlayerAttackState::TickWeapon(pPlayer_, fDeltaTime);
			}

			/**
			 * @brief 突き飛ばし（近接）。
			 * @details 移設元: Player::UpdateShove。
			 */
			void UpdateShove(float fDeltaTime)
			{
				(void)fDeltaTime;
			}

			/**
			 * @brief アクション（インタラクト等）。
			 * @details 移設元: Player::UpdateAction。
			 */
			void UpdateAction();


		private:
			Player* pPlayer_ = nullptr; //! 所有者（非所有）。
		};
	}
}