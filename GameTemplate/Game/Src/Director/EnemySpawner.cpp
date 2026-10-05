#include "stdafx.h"
#include "EnemySpawner.h"
#include "Src/Director/IEnemyPool.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Player.h"
#include "Src/System/GamePause.h"

namespace nsApp
{
	namespace nsDirector
	{
		bool EnemySpawner::Start()
		{
			/* ※この段階ではフラグをOFFにする。*/
			bSpawned_ = false;

			return true;
		}


		void EnemySpawner::Update()
		{
			/* ※一度出したら、もう出さない。*/
			if (nsSystem::IsGamePaused())
				return;

			/* ※一度出したら、もう出さない。*/
			TrySpawn();
		}


		void EnemySpawner::TrySpawn()
		{
			/* フラグが立たない場合、何もしない。*/
			if (bSpawned_)
				return;

			/* ※プールがセットされていない場合は、何もしない。*/
			if (pPool_ == nullptr)
				return;

			/* 対象を検索する。*/
			pTarget = FindGO<nsActor::Player>("player");
			if (pTarget == nullptr)
				return;

			const auto enemies = FindGOs < nsActor::CommonEnemy>("commonEnemy");
			if (enemies.empty())
				return;

			for (nsActor::CommonEnemy* pEnemy : enemies)
			{
				if (pEnemy == nullptr || !pEnemy->IsStart())
					continue;
			}

			/* 設定数だけ生成する。（枯渇したらそこで打ち切り）。*/
			for (int i = 0; i < iSpawnCount_; ++i)
			{
				/* プールから生成する。*/
				if (pPool_->Spawn(enType_, vSpawnPos_, pTarget) == nullptr)
					break;

				/* 生成したら、フラグを立てる。*/
				++iSpawned_;
			}

			/* 1体でも生成できたら、フラグを立てる。*/
			if(iSpawned_ > 0)
				bSpawned_ = true; 
		}
	}
}