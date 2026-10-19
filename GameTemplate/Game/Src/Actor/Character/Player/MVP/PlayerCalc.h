#pragma once

namespace nsApp
{
	namespace nsWeapon { class Weapon; }
	namespace nsActor
	{
		class CommonEnemy;

		/**
		 * @file   PlayerCalc.h
		 * @brief  操作キャラの計算系（MVP の計算）。
		 * @details 拡散・反動・ヒット・リロードモーション計算など、値を出す処理を担当する。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16
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
			void Init();

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
			void FindHitEnemies(const Vector3& vRayStart, const Vector3& vRayDirection, int iMaxCount, std::vector<struct HitEnemy>& vecOutHits);

			/**
			 * @brief ヒットスキャン1発を処理する。
			 * @details 移設元: Player::FireHitScan。
			 */
			void FireHitScan(nsWeapon::Weapon* pWeapon, const Vector3& vEyePos, const Vector3& vMuzzlePos, const Vector3& vAimDir);
		};
	}
}