#include "stdafx.h"
#include "EnemyStatusTable.h"

namespace
{
	const char* STATUS_FILE_PATH = "Assets/data/enemyStatus.tsv";
	const char* TYPE_COLUMN = "type";

	/**
	 * @brief 敵の種類名を、EnEnemyType へ変換する。知らない名前なら false を返す。
	 * @param sName 敵の名前。
	 * @param enOutType 変換結果の種類。
	 * @return 
	 */
	bool ToEnemyType(const std::string& sName, nsApp::nsActor::EnEnemyType& enOutType)
	{
		/* 既知の種類名を EnEnemyType へ変換する。*/
		if (sName == "Common") { enOutType = nsApp::nsActor::EnEnemyType::Common; return true; }
		if (sName == "Bile") { enOutType = nsApp::nsActor::EnEnemyType::Bile; return true; }
		if (sName == "Bomber") { enOutType = nsApp::nsActor::EnEnemyType::Bomber; return true; }

		return false;
	}

	std::unordered_map<nsApp::nsActor::EnEnemyType, nsApp::nsData::EnemyStatus> mapStatus_;
	bool bIsLoaded_ = false;
	nsApp::nsData::EnemyStatus stFallback_;
}


namespace nsApp
{
	namespace nsData
	{
		void EnemyStatusTable::Load()
		{
			mapStatus_.clear();
			bIsLoaded_ = true;

			/* 種類が重複しない表なので LoadTable。*/
			nsK2EngineLow::nsSystem::nsTSVFile::TSVTableLoader::LoadTable(STATUS_FILE_PATH, TYPE_COLUMN, mapStatus_,ToEnemyType);
		}


		const EnemyStatus& EnemyStatusTable::Get(nsActor::EnEnemyType enType)
		{
			/* 初回だけ TSV を読む。*/
			if (!bIsLoaded_)
				Load();

			/* 指定された種類のステータスを返す。*/
			const auto it = mapStatus_.find(enType);
			if (it == mapStatus_.end())
				return stFallback_;

			return it->second;
		}
	}
}