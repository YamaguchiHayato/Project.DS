#pragma once
#include "Src/Director/EnemySpawnSetting.h"

namespace nsApp
{
	namespace nsActor
	{
		class ICharacter;
		class Player;
	}

	namespace nsDirector
	{
		class IEnemyPool;

		/**
		 * @file   EnemySpawner.h
		 * @brief  配置点から敵をプール経由で出す係。見えない配置オブジェクト想定。
		 * @details プールは所有しない。Director などから IEnemyPool を受け取る。
		 *          湧き方(形・半径・間隔・数)は EnemySpawnSetting で決める。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/06: 最終更新日。
		 */
		class EnemySpawner : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			EnemySpawner() = default;
			virtual ~EnemySpawner() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;


		public:
			/**
			 * @brief 使用するプールをセットする。
			 * @param pPool 使用するプールのポインタ。
			 */
			inline void SetPool(IEnemyPool* pPool)
			{
				pPool_ = pPool;
			}

			/**
			 * @brief スポナーの設定をセットする(種類・中心・形・半径・間隔・数)。
			 * @param setting スポナーの設定。
			 */
			inline void SetSetting(const EnemySpawnSetting& setting)
			{
				stSetting_ = setting;
			}


		private:
			/**
			 * @brief プールの敵が全員初期化を終えたか確認する。終えていれば以降は確認しない。
			 * @return 湧かせてよければtrue。
			 */
			bool WaitReady();

			/**
			 * @brief 設定の数だけ1回分を湧かせる(プールが空、または合計に達したら打ち切る)。
			 */
			void SpawnWave();

			/**
			 * @brief 湧かせる位置を形に合わせて作る。
			 * @param index 1回分の中で何体目か(Ringの角度に使う)。
			 * @return 湧かせる位置。
			 */
			Vector3 MakeSpawnPosition(int index) const;

			/**
			 * @brief 合計の上限に達したか。
			 * @return 達していればtrue。上限が0(無制限)なら常にfalse。
			 */
			inline bool IsReachedTotal() const
			{
				return stSetting_.iTotal_ > 0 && iSpawned_ >= stSetting_.iTotal_;
			}


		private:
			IEnemyPool* pPool_ = nullptr; //! 使用するプール（非所有）。
			nsActor::Player* pTarget = nullptr; //! 追跡対象（非所有）。
			EnemySpawnSetting stSetting_; //! スポナーの設定。
			float fIntervalTimer_ = 0.0f; //! 次に湧くまでの残り時間。
			int iSpawned_ = 0; //! すでに出した数。
			bool bReady_ = false; //! プールの準備ができたか。
			bool bFinished_ = false; //! もう湧かせないか。
		};
	}
}