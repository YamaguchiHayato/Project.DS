#pragma once

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
		 * @date   2026/10/16
		 */
		class PlayerCombat
		{
		public:
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


		private:
			Player* pPlayer_ = nullptr; //! 所有者（非所有）。
			bool workWantFire_ = false; //! このフレーム撃つか。
			Vector3 workAimDir_ = Vector3::Zero; //! 照準方向。
			Vector3 workEyePos_ = Vector3::Zero; //! 目の位置。
			Vector3 workMuzzleRight_ = Vector3::Zero; //! 銃口の右方向。
			Vector3 workMuzzlePos_ = Vector3::Zero;	//! トレーサー始点。
			int workPelletIndex_ = 0; //! 散弾ループの添字。
			float workMotionRate_ = 0.0f;			//! 突き出し戻しの補間率。
			bool workIsFatigued_ = false;			//! 疲労中か。
			Vector3 workToEnemy_ = Vector3::Zero;	//! プレイヤー→敵(水平)。
			float workDist_ = 0.0f;					//! 敵までの距離。
			Vector3 workDir_ = Vector3::Zero;		//! 押し返し方向。
			Vector3 workShoveHitPos_ = Vector3::Zero;	//! 手応え演出の位置。
		};
	}
}