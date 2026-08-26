#include "stdafx.h"
#include "EnemyDirector.h"
#include "Player.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/System/GamePause.h"
#include <cstdlib>

namespace
{
	const float	fSpawnInterval_ = 2.0f;		//! 敵を湧かせる間隔(秒)。
	const int	iMaxAliveEnemies_ = 6;		//! 同時に存在できる敵の最大数。
	/*
	 * プレイヤーからどれだけ離して湧かせるか。
	 * CommonEnemy の発見距離(fDetectRange_=250)より内側に湧かせないと、
	 * 湧いた敵が待機のまま追ってこない。攻撃距離(fAttackRange_=120)よりは
	 * 外にして、湧いた瞬間に殴られないようにする。
	 * ※プレイヤーの移動速度(200)が敵の追跡速度(120)より速いため、
	 *   離れられると追いつけない。敵側のパラメータは要調整(山口担当領域)。
	 */
	const float	fSpawnRadius_ = 200.0f;
	const float	fPi_ = 3.14159265f;			//! 円周率。
}

namespace nsApp
{
	namespace nsDirector
	{
		bool EnemyDirector::Start()
		{
			/* 起動直後から間隔を数えて湧かせ始める。*/
			fSpawnTimer_ = 0.0f;
			return true;
		}


		void EnemyDirector::Update()
		{
			/* ポーズ中は湧かせない。*/
			if (nsSystem::IsGamePaused())
				return;

			/* プレイヤーがいなければ湧かせない。*/
			nsActor::Player* pPlayer = FindGO<nsActor::Player>("player");
			if (pPlayer == nullptr)
				return;

			/* 同時出現数が上限に達していれば湧かせない。*/
			const int iAlive = static_cast<int>(FindGOs<nsActor::CommonEnemy>("commonEnemy").size());
			if (iAlive >= iMaxAliveEnemies_)
				return;

			/* 一定間隔ごとに1体湧かせる。*/
			fSpawnTimer_ += g_gameTime->GetFrameDeltaTime();
			if (fSpawnTimer_ >= fSpawnInterval_)
			{
				fSpawnTimer_ = 0.0f;
				SpawnEnemy(pPlayer);
			}
		}


		void EnemyDirector::SpawnEnemy(nsActor::Player* pPlayer)
		{
			/* プレイヤーを中心に、ランダムな方角・一定距離の地点を湧き位置にする。*/
			const float fAngle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f * fPi_;
			Vector3 vSpawnPos = pPlayer->GetPosition();
			vSpawnPos.x += sinf(fAngle) * fSpawnRadius_;
			vSpawnPos.z += cosf(fAngle) * fSpawnRadius_;
			vSpawnPos.y = 0.0f;

			/*
			 * 敵を生成し、湧き位置へ置いてプレイヤーを標的にする。
			 * 座標を直接書き換えるだけだと敵の移動処理に上書きされてしまうため、
			 * 移動処理へも反映される SetPosition を使う。
			 */
			nsActor::CommonEnemy* pEnemy = NewGO<nsActor::CommonEnemy>(0, "commonEnemy");
			pEnemy->SetPosition(vSpawnPos);
			pEnemy->SetTarget(pPlayer);
		}
	}
}
