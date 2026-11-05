#pragma once
#include "Src/Director/EnemySpawnSetting.h"
#include "Src/System/RayTest/RaycastQuery.h"

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
		 *          湧き方(半径・間隔・作動距離・付近上限)は EnemySpawnSetting で決める。
		 *          プレイヤーが作動距離内にいる間だけ間隔ごとに無限湧きする(マイクラのスポナー寄せ)。
		 *          湧き位置は RaycastQuery で床を取り、遮蔽物の中・プレイヤー／他敵への重なりを避ける。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/06: 最終更新日。
		 * @date   2026/11/05: 無限湧き・作動距離対応。形と合計上限を廃止。
		 * @date   2026/11/05: 床レイと重なり回避を追加。
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
			 * @brief スポナーの設定をセットする(種類・中心・半径・間隔・数・作動距離・付近上限)。
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
			 * @brief プレイヤーが作動距離内にいるか(XZのみで測る)。
			 * @return 作動距離内ならtrue。
			 */
			bool IsPlayerInActivateRange() const;

			/**
			 * @brief 中心付近にいるアクティブな敵の数を数える。
			 * @return 付近の敵の数。
			 */
			int CountNearbyEnemies() const;

			/**
			 * @brief 設定の数だけ1回分を湧かせる(プールが空・安全な位置が無ければ打ち切る。次の間隔で再挑戦する)。
			 */
			void SpawnWave();

			/**
			 * @brief 円内のランダムなXZ候補を作る。Yは中心どおり(0)。床合わせはしない。
			 * @return 候補位置。
			 */
			Vector3 MakeCandidatePosition() const;

			/**
			 * @brief 候補の真上からレイを飛ばし、床の高さを取る。
			 * @param vCandidate 候補(主にXZ)。
			 * @param vOutGround 床ヒット位置(少し浮かせた足元)。
			 * @return 床に当たったらtrue。
			 */
			bool FindGroundPosition(const Vector3& vCandidate, Vector3& vOutGround) const;

			/**
			 * @brief プレイヤーに近すぎないか(XZ)。
			 * @param vPos 調べる位置。
			 * @return 近すぎたらtrue。
			 */
			bool IsTooCloseToPlayer(const Vector3& vPos) const;

			/**
			 * @brief すでに出ている敵に近すぎないか(XZ)。同じ場所に重ならないようにする。
			 * @param vPos 調べる位置。
			 * @return 近すぎたらtrue。
			 */
			bool IsTooCloseToEnemy(const Vector3& vPos) const;

			/**
			 * @brief 安全な湧き位置を探す。失敗したら横にずらして再試行する。
			 * @param vOutPos 採用した湧き位置。
			 * @return 見つかったらtrue。
			 */
			bool TryMakeValidSpawnPosition(Vector3& vOutPos) const;


		private:
			IEnemyPool* pPool_ = nullptr; //! 使用するプール（非所有）。
			nsActor::Player* pTarget_ = nullptr; //! 追跡対象（非所有）。
			EnemySpawnSetting stSetting_; //! スポナーの設定。
			nsSystem::RaycastQuery stRaycastQuery_; //! 床取り用のレイ問い合わせ。
			float fIntervalTimer_ = 0.0f; //! 次に湧くまでの残り時間。最初の1回は待たずに湧かせる。
			bool bReady_ = false; //! プールの準備ができたか。
			bool bDisabled_ = false; //! 種類未設定などで永久に湧かせないか。
		};
	}
}