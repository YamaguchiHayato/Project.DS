#pragma once

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
		 *         時間経過と同時出現数の上限を見て、プレイヤーの周囲に CommonEnemy を湧かせ続ける。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
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


		private:
			/**
			 * @brief プレイヤーの周囲に敵を1体湧かせて、プレイヤーを標的にする。
			 * @param pPlayer 標的にするプレイヤー。
			 */
			void SpawnEnemy(nsActor::Player* pPlayer);


		private:
			float fSpawnTimer_ = 0.0f;	//! 次の湧きまでの経過時間(秒)。
		};
	}
}
