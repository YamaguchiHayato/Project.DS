#include "stdafx.h"
#include "Src/Director/EnemySpawnSetting.h"

namespace
{
	/* 配置表の列名。*/
	const char* TYPE_COLUMN = "type"; //! ゾンビの種類名。
	const char* POS_X_COLUMN = "x"; //! 中心X。
	const char* POS_Z_COLUMN = "z"; //! 中心Z。
	const char* RADIUS_COLUMN = "radius"; //! 湧き円の半径。
	const char* INTERVAL_COLUMN = "interval"; //! 湧く間隔。
	const char* AMOUNT_COLUMN = "amount"; //! 1回に湧かせる数。
	const char* ACTIVATE_RANGE_COLUMN = "activateRange"; //! 作動距離。
	const char* MAX_NEARBY_COLUMN = "maxNearby"; //! 付近の敵の上限。

	/* 列が空のときの既定値。*/
	const float DEFAULT_RADIUS = 0.0f; //! 半径の既定。0なら中心そのもの。
	const float DEFAULT_INTERVAL = 2.0f; //! 間隔の既定(秒)。
	const int DEFAULT_AMOUNT = 1; //! 1回に湧かせる数の既定。
	const float DEFAULT_ACTIVATE_RANGE = 800.0f; //! 作動距離の既定(cm)。
	const int DEFAULT_MAX_NEARBY = 8; //! 付近上限の既定。

	/**
	 * @struct EnemyTypeName
	 * @brief  TSVに書く種類名と、ゾンビの種類の対応。
	 */
	struct EnemyTypeName
	{
		const char* pName; //! TSVに書く種類名。
		nsApp::nsActor::EnEnemyType enEnemyType; //! ゾンビの種類。
	};

	/* 種類を増やすときは、ここに1行足す。*/
	const EnemyTypeName ENEMY_TYPE_NAMES[] =
	{
		{ "Common", nsApp::nsActor::EnEnemyType::Common },
		{ "Bile", nsApp::nsActor::EnEnemyType::Bile },
		{ "Bomber", nsApp::nsActor::EnEnemyType::Bomber },
	};

	/**
	 * @brief TSVに書いた種類名をゾンビの種類へ変換する。
	 * @param typeName 種類名。
	 * @param outEnemyType 変換した種類。
	 * @return 知っている名前ならtrue。
	 */
	bool ToEnemyType(const std::string& typeName, nsApp::nsActor::EnEnemyType& outEnemyType)
	{
		/* 種類名の対応表を順に見ていく。*/
		for (const EnemyTypeName& entry : ENEMY_TYPE_NAMES)
		{
			/* 一致する名前が見つかったら種類を返す。*/
			if (typeName != entry.pName)
				continue;

			/* 一致した種類を返す。*/
			outEnemyType = entry.enEnemyType;
			return true;
		}

		return false;
	}
}


namespace nsApp
{
	namespace nsDirector
	{
		bool EnemySpawnSetting::Create(const nsK2EngineLow::nsSystem::nsTSVFile::TSVTable& table, int row, EnemySpawnSetting& outSetting)
		{
			/* 種類名が分からない行は使わない(打ち間違いで意図しない種類を湧かせない)。*/
			if (!ToEnemyType(table.GetString(row, TYPE_COLUMN), outSetting.enEnemyType))
				return false;

			/* 中心を読む。Yは使わないので常に0。空欄の列は既定値。*/
			outSetting.vCenter.Set(table.GetFloat(row, POS_X_COLUMN), 0.0f, table.GetFloat(row, POS_Z_COLUMN));

			/* 湧き方の数値を読む。空欄は既定値。*/
			outSetting.fRadius = table.GetFloat(row, RADIUS_COLUMN, DEFAULT_RADIUS);
			outSetting.fInterval = table.GetFloat(row, INTERVAL_COLUMN, DEFAULT_INTERVAL);
			outSetting.iAmount = table.GetInt(row, AMOUNT_COLUMN, DEFAULT_AMOUNT);
			outSetting.fActivateRange = table.GetFloat(row, ACTIVATE_RANGE_COLUMN, DEFAULT_ACTIVATE_RANGE);
			outSetting.iMaxNearby = table.GetInt(row, MAX_NEARBY_COLUMN, DEFAULT_MAX_NEARBY);

			/* 1回に湧かせる数が0以下、半径・間隔・作動距離が負の行は使わない。*/
			return outSetting.iAmount > 0 && outSetting.fRadius >= 0.0f && outSetting.fInterval >= 0.0f && outSetting.fActivateRange >= 0.0f;
		}
	}
}