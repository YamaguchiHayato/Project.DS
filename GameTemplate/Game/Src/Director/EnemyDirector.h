#pragma once
#include"EnemyPool.h"


namespace nsApp
{
	namespace nsActor
	{
		/* 前方宣言。*/
		class Player;
	}

	namespace nsDirector
	{
		/**
		 * @file   EnemyDirector.h
		 * @brief  雑魚敵の湧き(スポーン)を一元管理する係。L4D2のAI Directorの簡易版。
		 * @author Izumida Kiryu
		 * @date   2026/10:05: 最終更新日。
		 */
		class EnemyDirector : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			EnemyDirector() = default;
			virtual ~EnemyDirector() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;


		public:
			/**
			 * @brief 使用するプールを取得する。
			 * @return 使用するプールのポインタ。nullptr の場合はセットされていない。
			 */
			inline IEnemyPool* GetPool()
			{
				return &enemyPool_;
			}


		private:
			/**
			 * @brief プレイヤーの周囲に敵を1体湧かせて、プレイヤーを標的にする。
			 * @param pPlayer 標的にするプレイヤー。
			 */
			void SpawnEnemy(nsActor::Player* pPlayer);


		private:
			float fSpawnTimer_ = 0.0f;	//! 次の湧きまでの経過時間(秒)。
			EnemyPool enemyPool_;	//! 敵を生成するためのスポナー用プール。
		};
	}
}
