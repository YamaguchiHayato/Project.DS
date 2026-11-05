#include "stdafx.h"
#include "EnemyDirector.h"
#include "Src/Director/EnemySpawner.h"

namespace
{
	/* 射撃場と同じ配置表。ステージを増やすときはパスを切り替えるか、stage列付き表にする。*/
	const char* ENEMY_SPAWN_FILE_PATH = "Assets/data/enemySpawn_stage1.tsv";
}


namespace nsApp
{
	namespace nsDirector
	{
		EnemyDirector::~EnemyDirector()
		{
			/* スポナーを先に消して、以降の湧きを止める。*/
			for (EnemySpawner* pSpawner : vecSpawners_)
			{
				if (pSpawner != nullptr)
					DeleteGO(pSpawner);
			}
			vecSpawners_.clear();
		}


		bool EnemyDirector::Start()
		{
			/* プールを用意してから、射撃場と同じ表でスポナーを立てる。*/
			enemyPool_.Init();
			SpawnFromTable();

			return true;
		}


		void EnemyDirector::Update()
		{
			/* 湧きの進行は各 EnemySpawner の Update に任せる。*/
		}


		void EnemyDirector::SpawnFromTable()
		{
			/* 配置表を読む。読めなければ敵は置かない(失敗はウィンドウで知らせる)。*/
			std::vector<EnemySpawnSetting> vecSettings;
			if (!nsK2EngineLow::nsSystem::nsTSVFile::TSVTableLoader::LoadList(ENEMY_SPAWN_FILE_PATH, vecSettings))
				return;

			/* 1行につき1つスポナーを置く。*/
			for (const EnemySpawnSetting& setting : vecSettings)
			{
				EnemySpawner* pSpawner = NewGO<EnemySpawner>(0, "enemySpawner");
				pSpawner->SetPool(&enemyPool_);
				pSpawner->SetSetting(setting);
				vecSpawners_.push_back(pSpawner);
			}
		}
	}
}