#pragma once
#include "Src/Director/IEnemyPool.h"


namespace nsApp
{
	namespace nsActor {
		class CommonEnemy;
		class BomberEnemy;
	}


	namespace nsDirector
	{
		/**
		 * @file EnemyPool.h
		 * @brief 敵プールの実装。
		 * @todo: 他のゾンビの種類は後ほど実装。現在はCommonEnemyのみ。
		 * @outer: Yamaguchi Hayato
		 * @date: 2026/10/02
		 */
		class EnemyPool : public IEnemyPool
		{
		public:
			/*コンストラクタとデストラクタ。 */
			EnemyPool() = default;
			virtual ~EnemyPool() = default;


		public:
			/**
			 * @brief 敵キャラクターを生成する。
			 * @param iPoolSize プールのサイズ。
			 */
			void Init() override;

			/**
			 * @brief 敵キャラクターを生成する。
			 * @param enType 生成するモデルの種類。
			 * @param vPosition 生成位置。
			 * @param pTarget 追跡対象。
			 * @return 生成した敵キャラクターのポインタ。失敗した場合は nullptr を返す。
			 */
			nsActor::IEnemy* Spawn(CharacterModelType enType, const Vector3& vPosition, nsActor::ICharacter* pTarget) override;

			/**
			 * @brief 敵キャラクターを生成する。
			 * @param pEnemy 生成対象のポインタ。
			 */
			void Release(nsActor::IEnemy* pEnemy) override;

			/**
			 * @brief ステージ上に生成しているゾンビの数を取得する。
			 * @return 生成数。
			 */
			int ActiveCount() const override;

			/**
			 * @brief プール内の敵が全員 Start を終えたか。
			 * @return 全員終わっていれば true。
			 */
			bool AreAllStarted() const override;

			/**
			 * @brief 中心から半径内にいるアクティブな敵の数を返す。
			 * @param vCenter 中心座標。
			 * @param fRange 半径(cm)。
			 * @return 付近の敵の数。
			 */
			int CountNear(const Vector3& vCenter, float fRange) const override;

			/**
			 * @brief 指定位置の近くにアクティブな敵がいるか。
			 * @param vPos 調べる位置。
			 * @param fClearDistance 近いとみなす距離(cm)。
			 * @return 近すぎる敵がいれば true。
			 */
			bool HasEnemyNear(const Vector3& vPos, float fClearDistance) const override;


		private:
			static const int iPoolSize_ = 20; //! プールのサイズ。
			static const int iBomberPoolSize_ = 4;
			nsActor::CommonEnemy* pCommonPoolNum_[iPoolSize_] = {};
			nsActor::BomberEnemy* pBomberPoolNum_[iBomberPoolSize_] = {};
			bool bInited_ = false;
		};
	}
}

