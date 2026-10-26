#pragma once
#include "IAttackStrategy.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerAttackState.h
		 * @brief  操作キャラの銃戦闘ステート。
		 * @details 敵の AttackState とは別。将来ここで射撃・ADS などの挙動をまとめる。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/20
		 */
		class AttackFireStrategy : public IAttackStrategy
		{
		public:
			/**
			 * @brief この機能を1フレ分実行する。
			 * @param pPlayer    実行対象。
			 * @param fDeltaTime 経過時間(秒)。使わない戦略は無視してよい。
			 */
			void Execute(Player* pPlayer, float fDeltaTime) override;


		private:
			bool workWantFire_ = false; //! 射撃したいか。Player::stIntent_.bFireTrigger_ からコピーする。
			Vector3 workAimDir_ = Vector3::Zero; //! 照準方向。Player::GetLookDirection() からコピーする。
			Vector3 workEyePos_ = Vector3::Zero; //! 目の位置。Player::vPosition_ + Player::stCharacterStatus_.stEyeOffset_ からコピーする。
			Vector3 workMuzzleRight_ = Vector3::Zero; //! 銃口の右方向。Player::stCharacterStatus_.stEyeOffset_ からコピーする。
			Vector3 workMuzzlePos_ = Vector3::Zero; //! 銃口の位置。Player::vPosition_ + Player::stCharacterStatus_.stEyeOffset_ からコピーする。
			int workPelletIndex_ = 0; //! 散弾の何発目か。Player::stWeaponInventory_.GetCurrentWeapon()->GetPelletCount() からコピーする。
			float workMuzzleSideSign_ = 1.0f; //! 銃口を横へずらす向き。右なら+1、左なら-1。二丁持ちのとき撃つたびに入れ替わる。
		};
	}
}