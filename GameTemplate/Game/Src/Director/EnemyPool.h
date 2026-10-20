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


		private:
			static const int iPoolSize_ = 20; //! プールのサイズ。
			static const int iBomberPoolSize_ = 4;
			nsActor::CommonEnemy* pCommonPoolNum_[iPoolSize_] = {};
			nsActor::BomberEnemy* pBomberPoolNum_[iBomberPoolSize_] = {};
			bool bInited_ = false;
		};
	}
}

