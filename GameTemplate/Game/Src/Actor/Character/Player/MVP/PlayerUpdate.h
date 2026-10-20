#pragma once

namespace nsApp
{
	namespace nsActor
	{
		/* 前方宣言。cpp で Player.h を include する。*/
		class Player;
		struct PlayerIntent;

		/**
		 * @file   PlayerUpdate.h
		 * @brief  操作キャラの常時更新系（MVP の更新）。
		 * @details 視点・ADS補間・体力・目の高さなど、毎フレ進める処理を担当する。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/19: 更新日。
		 */
		class PlayerUpdate
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PlayerUpdate() = default;
			virtual ~PlayerUpdate() = default;


		public:
			/**
			 * @brief 所有者を結びつける。
			 * @param pPlayer 操作キャラ（非所有）。
			 */
			inline void Bind(Player* pPlayer)
			{
				pPlayer_ = pPlayer;
			}

			/**
			 * @brief 視点を回す。
			 * @param stIntent このフレームの操作意図。
			 * @details 移設元: Player::UpdateLook。
			 */
			void UpdateLook(const PlayerIntent& stIntent);

			/**
			 * @brief 姿勢・移動による拡散倍率を進める。
			 * @details 移設元: Player::UpdateStanceSpread。
			 */
			void UpdateStanceSpread(float fDeltaTime);

			/**
			 * @brief 反動の減衰を進める。
			 * @details 移設元: Player::UpdateRecoil。
			 */
			void UpdateRecoil(float fDeltaTime);

			/**
			 * @brief 視点の揺れを進める。
			 * @param stIntent このフレームの操作意図。
			 * @pararm fDeltaTime 1フレームの経過時間(秒)。
			 * @details 移設元: Player::UpdateViewSway。
			 */
			void UpdateViewSway(float fDeltaTime, const PlayerIntent& stIntent);

			/**
			 * @brief 体力の時間経過を進める。
			 * @details 移設元: Player::UpdateHealth。
			 */
			void UpdateHealth(float fDeltaTime);

			/**
			 * @brief 目の高さを進める。
			 * @details 移設元: Player::UpdateEyeHeight。
			 */
			void UpdateEyeHeight(float fDeltaTime);

			/**
			 * @brief 生命状態を更新する。
			 * @details 移設元: Player::UpdateLifeState。
			 */
			void UpdateLifeState(float fDeltaTime);

			/**
			 * @brief 移動を進める。
			 * @details 移設元: Player::UpdateMove。
			 */
			void UpdateMove(float fDeltaTime, const PlayerIntent& stIntent);


		private:
			/**
			 * @brief 覗き込み度合いを目標へ寄せる。
			 */
			void ApproachAds(float fDeltaTime, const PlayerIntent& stIntent);

			/**
			 * @brief 銃の下げ度合いを目標へ寄せる。
			 */
			void ApproachLower(float fDeltaTime);

			/**
			 * @brief 連射拡散を時間で収める。
			 */
			void RecoverSpread(float fDeltaTime);

			/**
			 * @brief 横移動に合わせたカメラの傾き(ストレイフロール)を進める。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 * @param stIntent このフレームの操作意図。
			 */
			void UpdateStrafeRoll(float fDeltaTime, const PlayerIntent& stIntent);

			/**
			 * @brief 武器の揺れを進める。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 * @param stIntent このフレームの操作意図。
			 */
			void UpdateWeaponSway(float fDeltaTime, const PlayerIntent& stIntent);

			/**
			 * @brief 歩きに合わせたカメラの上下動と傾きを進める。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateBobWeight(float fDeltaTime);

			/**
			 * @brief 横移動に合わせたカメラの傾き(ストレイフロール)を進める。
			 */
			void UpdateViewBob();


		private:
			Player* pPlayer_ = nullptr; //! 所有者（非所有）。
			float workBlendRate_ = 0.0f; //! 寄せ速度の組み立て用。
			float workStanceTarget_ = 1.0f; //! 姿勢拡散の目標倍率。
			float workShakeRate_ = 0.0f; //! 覗き込みで抑えた揺れ倍率。
			float workBobShake_ = 0.0f; //! 歩行ボブに掛ける揺れ倍率。
			float workBobRoll_ = 0.0f; //! 歩行でカメラが傾く量。
			Vector3 workCameraForward_ = Vector3::Zero; //! カメラの前方向ベクトル。
			Vector3 workCameraRight_ = Vector3::Zero; //! カメラの右方向ベクトル。
			Vector3 workMoveDir_ = Vector3::Zero; //! 移動方向ベクトル。
			float workMoveSpeed_ = 0.0f; //! 移動速度。
		};
	}
}