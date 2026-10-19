#pragma once

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerCombat.h
		 * @brief  銃・近接など Attack 系の実処理置き場。
		 * @details ADS / 射撃 / リロード / 近接の中身は後でここへ移し、
		 *          AttackState から呼ぶ。メイン／サブは Strategy で切り替える。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16
		 */
		class PlayerCombat
		{
		public:
			PlayerCombat() = default;
			virtual ~PlayerCombat() = default;


		public:
			/**
			 * @brief 戦闘処理の初期化。
			 */
			void Init();

			/**
			 * @brief 武器更新（射撃・リロード等）。
			 * @details 移設元: Player::UpdateWeapon。後で Attack 内サブステートへ分割する。
			 */
			void UpdateWeapon(float fDeltaTime);

			/**
			 * @brief 突き飛ばし（近接）。
			 * @details 移設元: Player::UpdateShove。
			 */
			void UpdateShove(float fDeltaTime);

			/**
			 * @brief アクション（インタラクト等）。
			 * @details 移設元: Player::UpdateAction。
			 */
			void UpdateAction();
		};
	}
}