#pragma once
#include "Weapon.h"


namespace nsApp
{
	namespace nsItem { 
		class Pickup; 
	}

	namespace nsActor
	{
		class Player;	

		/**
		 * @file   PlayerItem.h
		 * @brief  所持アイテム（攻撃系／回復系）。
		 * @details 攻撃系（爆弾等）は Attack 寄りから、回復系は Attack以外から使う。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/19: 最終更新日。
		 */
		class PlayerItem
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PlayerItem() = default;
			virtual ~PlayerItem() = default;


		public:
			/**
			 * @brief プレイヤーを紐付ける。
			 * @param pPlayer 紐付けるプレイヤー。
			 */
			inline void Bind(Player* pPlayer)
			{
				pPlayer_ = pPlayer;
			}

			/**
			 * @brief アイテム更新。
			 * @details 移設元: Player::UpdateItems。
			 */
			void UpdateItems(float fDeltaTime);

			/**
			 * @brief メディキット使用。
			 * @details 移設元: Player::UpdateMedkit（回復系）。
			 */
			void UpdateMedkit(float fDeltaTime);

			/**
			 * @brief 即効アイテム使用。
			 * @details 移設元: Player::UseQuickItem（回復系）。
			 */
			void UseQuickItem();

			/**
			 * @brief 拾い上げ。
			 * @details 移設元: Player::PickUpItem。
			 */
			void PickUpItem();

			/**
			 * @brief 落ちている銃と手持ちを入れ替える。
			 * @details 移設元: Player::SwapWeaponWithPickup。
			 */
			bool SwapWeaponWithPickup(nsItem::Pickup* pPickup);


		private:
			Player* pPlayer_ = nullptr; //! 所有者（非所有）。
			Vector3 workLook_ = Vector3::Zero;	//! 投擲の視線方向。
			Vector3 workThrowPos_ = Vector3::Zero;	//! 投擲の始点。
			bool workCanHeal_ = false;//! メドキットを使えるか。
			bool workWantHeal_ = false;//! 使おうとしているか。
			int workMissing_ = 0; //! 失っている恒久HP。
			int workHeal_ = 0; //! 回復量。
			Vector3 workHealPos_ = Vector3::Zero; //! 回復演出の位置。
			int workGain_ = 0;	//! 即効アイテムの一時体力増加量。
			bool workPickedUp_ = false;	//! この物資を拾えたか。
			nsWeapon::EnWeaponType workOldWeaponType_ = nsWeapon::EnWeaponType::Handgun; //! 手放した銃の種類。
		};
	}
}