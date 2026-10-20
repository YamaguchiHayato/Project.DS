#pragma once
#include "Src/Combat/HitBoxSet.h"


namespace nsApp
{
	namespace nsWeapon {
		class Weapon; 
	}

	namespace nsActor
	{
		class Player;
		class CommonEnemy;

		/**
		 * @struct HitEnemy
		 * @brief  1本の弾道に当たった敵1体ぶんの記録。
		 */
		struct HitEnemy
		{
			CommonEnemy* pEnemy_ = nullptr;		//! 当たった敵。
			nsCombat::HitResult stResult_;		//! 当たった部位と位置。
		};

		/**
		 * @file   PlayerCalc.h
		 * @brief  操作キャラの計算系（MVP の計算）。
		 * @details 拡散・反動・ヒット・リロードモーション計算など、値を出す処理を担当する。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/19: 最終更新日。
		 */
		class PlayerCalc
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PlayerCalc() = default;
			virtual ~PlayerCalc() = default;


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
			 * @brief 現在の拡散角を取得する。
			 * @details 移設元: Player::GetCurrentSpread。
			 */
			float GetCurrentSpread() const;

			/**
			 * @brief 照準方向に拡散をかけた弾の向きを作る。
			 * @details 移設元: Player::MakeSpreadDirection。
			 */
			Vector3 MakeSpreadDirection(const Vector3& vAimDir) const;

			/**
			 * @brief リロード動作の区切り値を計算する。
			 * @details 移設元: Player::CalcReloadMotion。
			 */
			void CalcReloadMotion(float fReloadRate, float& fOutLower, float& fOutInsert, float& fOutSettle) const;

			/**
			 * @brief 射撃1発ぶんの反動を加える。
			 * @details 移設元: Player::ApplyFireRecoil。
			 */
			void ApplyFireRecoil(nsWeapon::Weapon* pWeapon);

			/**
			 * @brief レイ上の敵を集める。
			 * @details 移設元: Player::FindHitEnemies。
			 */
			void FindHitEnemies(const Vector3& vRayStart, const Vector3& vRayDirection, int iMaxCount, std::vector<HitEnemy>& vecOutHits);

			/**
			 * @brief ヒットスキャン1発を処理する。
			 * @details 移設元: Player::FireHitScan。
			 */
			void FireHitScan(nsWeapon::Weapon* pWeapon, const Vector3& vEyePos, const Vector3& vMuzzlePos, const Vector3& vAimDir);


		private:
			Player* pPlayer_ = nullptr; //! 所有者（非所有）。
			mutable float workAngle_ = 0.0f; //! ランダムな角度。
			mutable float workRadius_ = 0.0f; //! ランダムな半径。
			mutable Vector3 workRight_ = Vector3::Zero; //! ランダムな右方向。
			mutable Vector3 workUp_ = Vector3::Zero; //! ランダムな上方向。
			mutable Vector3 workResult_ = Vector3::Zero; //! ランダムな結果の方向。
			float workAdsMul_ = 0.0f; //! 覗き込み中の反動倍率の作業用。
			HitEnemy workHit_; //! 敵1体ぶんの命中記録の作業用。
			Vector3 workShotDir_ = Vector3::Zero;	//! 拡散後の弾道方向。
			Vector3 workHitPoint_ = Vector3::Zero;	//! レイ終点／命中点。
			std::vector<HitEnemy> workHits_; //! 命中した敵の一覧。
			int workHitIndex_ = 0; //! 命中ループの添字。
			int workDamage_ = 0; //! 1発ぶんのダメージ。
		};
	}
}