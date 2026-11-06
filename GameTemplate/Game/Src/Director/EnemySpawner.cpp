#include "stdafx.h"
#include "EnemySpawner.h"
#include "Src/Director/IEnemyPool.h"
#include "Src/Actor/Character/Common/CharacterModel.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Actor/Character/Enemy/BomberEnemy.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Src/System/GamePause.h"

namespace
{
	const float TWO_PI = 6.2831853f; //! 一周の角度(ラジアン)。
	const float NEARBY_RANGE_MIN = 100.0f; //! 付近判定の最小半径。radius が小さいとき用。
	const int SPAWN_RETRY_COUNT = 24; //! オブジェクト内なら捨てて、別のランダムXZを引き直す回数。
	const float SPAWN_RAY_UP = 500.0f; //! 床レイの始点を候補より上へずらす量(cm)。
	const float SPAWN_RAY_DOWN = 1000.0f; //! 床レイの終点を候補より下へずらす量(cm)。
	const float SPAWN_GROUND_OFFSET = 2.0f; //! 床めり込み防止で足元を少し上げる量(cm)。
	const float SPAWN_PLAYER_CLEAR = 80.0f; //! プレイヤーとの最短XZ距離(cm)。
	const float SPAWN_ENEMY_CLEAR = 40.0f; //! 他の敵との最短XZ距離(cm)。
	const float SPAWN_CLEAR_UP = 10.0f; //! 頭上レイの始点を足元から上げる量。
	const float SPAWN_HEAD_ROOM = 160.0f; //! 頭上クリアランスの長さ。
	const float SPAWN_HEAD_HIT_MAX = 150.0f; //! これより手前に天井があれば埋め込み。
	const float SPAWN_MAX_HEIGHT_ABOVE_CENTER = 80.0f; //! スポナー中心よりこれ以上高い床は屋根扱い。
	const float SPAWN_BODY_HEIGHT = 90.0f; //! 水平レイの高さ(足元から)。
	const float SPAWN_OUTSIDE_DISTANCE = 600.0f; //! オブジェクトの外側から飛ばす距離。コンテナ等より長くする。
	const float SPAWN_BLOCK_SLACK = 25.0f; //! 体に届く直前の余裕。これより手前のヒットは「物が間にある」。
	const int SPAWN_BLOCK_DIR_COUNT = 8; //! 水平に見る方向数。
	const int SPAWN_BLOCK_DIR_MIN = 3; //! この数以上の方向で物が間にあれば、オブジェクトの中とみなす。


	/**
	 * @brief 0〜1の乱数を返す。
	 * @return 0〜1の乱数。
	 */
	float Random01()
	{
		return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

	/**
	 * @brief ゾンビの種類を、プールに渡すモデルの種類へ変換する。
	 * @param enEnemyType ゾンビの種類。
	 * @param outModelType 変換したモデルの種類。
	 * @return 変換できたらtrue。未設定(None)ならfalse。
	 */
	bool ToCharacterModelType(nsApp::nsActor::EnEnemyType enEnemyType, nsApp::CharacterModelType& outModelType)
	{
		/* 未設定(None)は変換できない。*/
		switch (enEnemyType)
		{
		case nsApp::nsActor::EnEnemyType::Common:
			outModelType = nsApp::CharacterModelType::Common;
			return true;

		case nsApp::nsActor::EnEnemyType::Bile:
			outModelType = nsApp::CharacterModelType::Bile;
			return true;

		case nsApp::nsActor::EnEnemyType::Bomber:
			outModelType = nsApp::CharacterModelType::Bomber;
			return true;

		default:
			return false;
		}
	}

	/**
	 * @brief 2点のXZ距離を返す。
	 * @param vA 一方の座標。
	 * @param vB もう一方の座標。
	 * @return XZ平面上の距離。
	 */
	float DistanceXZ(const Vector3& vA, const Vector3& vB)
	{
		const float fDx = vA.x - vB.x;
		const float fDz = vA.z - vB.z;
		return sqrtf(fDx * fDx + fDz * fDz);
	}
}


namespace nsApp
{
	namespace nsDirector
	{
		bool EnemySpawner::Start()
		{
			/* 種類が未設定なら何も湧かせない(設定漏れで勝手に雑魚が湧かないように)。*/
			if (stSetting_.enEnemyType == nsActor::EnEnemyType::None)
				bDisabled_ = true;

			return true;
		}


		void EnemySpawner::Update()
		{
			/* ポーズ中は止める(間隔のタイマーも進めない)。*/
			if (nsSystem::IsGamePaused())
				return;

			/* 無効、または準備ができていなければ何もしない。*/
			if (bDisabled_ || !WaitReady())
				return;

			/* プレイヤーが離れている間は湧かせない。また近づいたら再開する。*/
			if (!IsPlayerInActivateRange())
				return;

			/* 付近の敵が上限以上なら、この回は湧かせない(無限でもゲームが埋まらないように)。*/
			if (stSetting_.iMaxNearby > 0 && CountNearbyEnemies() >= stSetting_.iMaxNearby)
				return;

			/* 間隔が0なら毎フレーム試す。それ以外はタイマーで待つ。最初の1回は待たない。*/
			if (stSetting_.fInterval > 0.0f)
			{
				fIntervalTimer_ -= g_gameTime->GetFrameDeltaTime();
				if (fIntervalTimer_ > 0.0f)
					return;

				fIntervalTimer_ += stSetting_.fInterval;
			}

			/* 湧かせる。*/
			SpawnWave();
		}


		bool EnemySpawner::WaitReady()
		{
			/* 一度準備ができたら、以降は確認しない。*/
			if (bReady_)
				return true;

			/* プール未設定、またはプレイヤー未セットなら待つ。*/
			if (pPool_ == nullptr || pTarget_ == nullptr)
				return false;

			/* プール内の敵が全員 Start するまで待つ(FindGOs は使わない)。*/
			if (!pPool_->AreAllStarted())
				return false;

			bReady_ = true;
			return true;
		}


		bool EnemySpawner::IsPlayerInActivateRange() const
		{
			/* 対象がいなければ湧かせない。*/
			if (pTarget_ == nullptr)
				return false;

			/* 湧かせる範囲内にプレイヤーがいれば湧かせる。*/
			return DistanceXZ(pTarget_->GetPosition(), stSetting_.vCenter) <= stSetting_.fActivateRange;
		}


		int EnemySpawner::CountNearbyEnemies() const
		{
			/* 付近判定の半径。湧き円が小さすぎるとすぐ上限に達するので下限を設ける。*/
			const float fNearbyRange = (stSetting_.fRadius > NEARBY_RANGE_MIN) ? stSetting_.fRadius : NEARBY_RANGE_MIN;

			/* 世界検索せず、プールに付近の人数を聞く。*/
			return pPool_->CountNear(stSetting_.vCenter, fNearbyRange);
		}


		void EnemySpawner::SpawnWave()
		{
			/* プールに渡すモデルの種類へ変換する。変換できなければ湧かせない。*/
			CharacterModelType enModelType = CharacterModelType::Common;
			if (!ToCharacterModelType(stSetting_.enEnemyType, enModelType))
				return;

			/* 指定された数だけ湧かせる。プールが空・安全位置が無ければ打ち切る。*/
			for (int i = 0; i < stSetting_.iAmount; ++i)
			{
				Vector3 vSpawnPos = Vector3::Zero;
				if (!TryMakeValidSpawnPosition(vSpawnPos))
					continue;

				/* プールが空なら打ち切る(次の間隔で再挑戦する)。*/
				if (pPool_->Spawn(enModelType, vSpawnPos, pTarget_) == nullptr)
					return;
			}
		}


		Vector3 EnemySpawner::MakeCandidatePosition() const
		{
			/* 湧かせる位置の初期値は中心(Yは0)。*/
			Vector3 vPos = stSetting_.vCenter;

			/* 半径0なら中心そのもの。*/
			if (stSetting_.fRadius <= 0.0f)
				return vPos;

			/* 円の中に偏りなく散らす(半径に√を掛けないと中心に寄る)。*/
			const float fAngle = Random01() * TWO_PI;
			const float fLength = sqrtf(Random01()) * stSetting_.fRadius;
			vPos.x += cosf(fAngle) * fLength;
			vPos.z += sinf(fAngle) * fLength;

			return vPos;
		}


		bool EnemySpawner::FindGroundPosition(const Vector3& vCandidate, Vector3& vOutGround) const
		{
			/* 候補の真上から真下へレイを飛ばし、ステージコリジョン／床に当てる。*/
			Vector3 vFrom = vCandidate;
			vFrom.y += SPAWN_RAY_UP;

			Vector3 vTo = vCandidate;
			vTo.y -= SPAWN_RAY_DOWN;

			const nsSystem::RaycastHit stHit = stRaycastQuery_.Trace(vFrom, vTo);
			if (!stHit.bHit_)
				return false;

			/*
			 * スポナー中心より明らかに高い面はコンテナ屋根などとみなして捨てる。
			 * 「下にもう一面ある＝即NG」は厚い床メッシュで全滅しやすいので使わない。
			 */
			if (stHit.vHitPos_.y - stSetting_.vCenter.y > SPAWN_MAX_HEIGHT_ABOVE_CENTER)
				return false;

			/* ヒット位置を足元にする。少し浮かせて床めり込みを防ぐ。*/
			vOutGround = stHit.vHitPos_;
			vOutGround.y += SPAWN_GROUND_OFFSET;
			return true;
		}


		bool EnemySpawner::IsTooCloseToEnemy(const Vector3& vPos) const
		{
			/* 同じ場所への重なりを避ける。判定本体はプール側。*/
			return pPool_->HasEnemyNear(vPos, SPAWN_ENEMY_CLEAR);
		}


		bool EnemySpawner::IsTooCloseToPlayer(const Vector3& vPos) const
		{
			/* 対象がいなければ近さ判定しない(湧かせない側は WaitReady / 作動距離で見る)。*/
			if (pTarget_ == nullptr)
				return false;

			/* プレイヤーとのXZ距離が近すぎる位置は捨てる。*/
			return DistanceXZ(vPos, pTarget_->GetPosition()) < SPAWN_PLAYER_CLEAR;
		}


		bool EnemySpawner::TryMakeValidSpawnPosition(Vector3& vOutPos) const
		{
			/* 再試行しても安全な位置が見つからない場合は湧かせない。*/
			for (int iTry = 0; iTry < SPAWN_RETRY_COUNT; ++iTry)
			{
				/* 毎回円内ランダム。不合格ならXZの引き直しだけで、ランダム仕様は変えない。*/
				const Vector3 vCandidate = MakeCandidatePosition();

				/* 床が取れない＝空洞や範囲外。次の候補へ。*/
				Vector3 vGround = Vector3::Zero;
				if (!FindGroundPosition(vCandidate, vGround))
					continue;

				/* ステージオブジェクトの中・壁の厚みの中なら捨てる。*/
				if (IsEmbeddedInObstacle(vGround))
					continue;

				/* プレイヤーや他の敵に近すぎる位置は捨てる。*/
				if (IsTooCloseToPlayer(vGround))
					continue;
				if (IsTooCloseToEnemy(vGround))
					continue;

				vOutPos = vGround;
				return true;
			}

			return false;
		}


		bool EnemySpawner::IsEmbeddedInObstacle(const Vector3& vGround) const
		{
			/* 頭上にすぐ天井があれば棚下・閉じた箱の中。*/
			Vector3 vUpFrom = vGround;
			vUpFrom.y += SPAWN_CLEAR_UP;
			Vector3 vUpTo = vUpFrom;
			vUpTo.y += SPAWN_HEAD_ROOM;
			const nsSystem::RaycastHit stUp = stRaycastQuery_.Trace(vUpFrom, vUpTo);
			if (stUp.bHit_ && stUp.fDistance_ < SPAWN_HEAD_HIT_MAX)
				return true;

			/*
			 * 体の位置からレイを出すと、壁の厚みの中にいるとき始点がコリジョン内になり、
			 * ヒットしない(今のコンテナめり込みの原因)。
			 * 外側から体へ飛ばし、「体より手前で物に当たる方向」が多ければオブジェクト内。
			 * コンテナでも棚でも同じ。XZのランダムは変えない。
			 */
			Vector3 vBody = vGround;
			vBody.y += SPAWN_BODY_HEIGHT;

			int iBlocked = 0;
			for (int i = 0; i < SPAWN_BLOCK_DIR_COUNT; ++i)
			{
				const float fAngle = TWO_PI * static_cast<float>(i) / static_cast<float>(SPAWN_BLOCK_DIR_COUNT);
				Vector3 vFrom = vBody;
				vFrom.x += cosf(fAngle) * SPAWN_OUTSIDE_DISTANCE;
				vFrom.z += sinf(fAngle) * SPAWN_OUTSIDE_DISTANCE;

				const nsSystem::RaycastHit stHit = stRaycastQuery_.Trace(vFrom, vBody);
				if (!stHit.bHit_)
					continue;

				/* ヒットが体に届く前＝その方向にステージオブジェクトがある。*/
				const float fDistToBody = DistanceXZ(vFrom, vBody);
				if (stHit.fDistance_ < fDistToBody - SPAWN_BLOCK_SLACK)
					++iBlocked;
			}

			/* 近くに壁が1面だけなら通路脇。複数方向ブロックなら物の中。*/
			return iBlocked >= SPAWN_BLOCK_DIR_MIN;
		}	}
}