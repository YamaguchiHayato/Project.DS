#pragma once
#include "EnemyPool.h"
#include "Src/Director/EnemySpawnSetting.h"
#include <vector>

namespace nsApp
{
	namespace nsActor
	{
		class Player;
	}


	namespace nsDirector
	{
		class EnemySpawner;

		/**
		 * @file   EnemyDirector.h
		 * @brief  雑魚敵の湧きを一元管理する係。
		 * @details プールを持ち、配置表(TSV)の行ごとに EnemySpawner を立てる。
		 *          射撃場(DebugShootingRangeScene)と同じ表・同じ湧き方を本編でも使う。
		 * @author Izumida Kiryu
		 * @date   2026/10/05: 最終更新日。
		 * @date   2026/11/05: 間隔直湧きをやめ、TSV＋EnemySpawner 方式に統一。
		 */
		class EnemyDirector : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			EnemyDirector() = default;
			virtual ~EnemyDirector();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;


		public:
			/**
			 * @brief 使用するプールを取得する。
			 * @return 使用するプールのポインタ。
			 */
			inline IEnemyPool* GetPool()
			{
				return &enemyPool_;
			}


		public:
			/**
			 * @brief 全スポナーに追跡対象のプレイヤーを渡す。
			 * @details スポナー側で FindGO しないための入口。生成後でも呼び直せる。
			 * @param pTarget プレイヤー(非所有)。
			 */
			void SetTarget(nsActor::Player* pTarget);


		private:
			/**
			 * @brief 配置表を読み、行ごとにスポナーを置く。
			 */
			void SpawnFromTable();


		private:
			nsActor::Player* pTarget_ = nullptr; //! 追跡対象のプレイヤー(非所有)。
			EnemyPool enemyPool_; //! 敵を生成するためのスポナー用プール。
			std::vector<EnemySpawner*> vecSpawners_; //! 配置表の行ごとに置いたスポナー。
		};
	}
}