#pragma once

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerItem.h
		 * @brief  所持アイテム（攻撃系／回復系）。
		 * @details 攻撃系（爆弾等）は Attack 寄りから、回復系は Attack以外から使う。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16
		 */
		class PlayerItem
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PlayerItem() = default;
			virtual ~PlayerItem() = default;


		public:
			/**
			 * @brief アイテム処理の初期化。
			 */
			void Init();

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
		};
	}
}