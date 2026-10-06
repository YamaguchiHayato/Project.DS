#include "stdafx.h"
#include "Src/Director/EnemySpawnSetting.h"

namespace
{
	/* 配置表の列名。*/
	const char* sTypeColumn_ = "type"; //! ゾンビの種類名。
	const char* sPosXColumn_ = "x"; //! 中心X。
	const char* sPosYColumn_ = "y"; //! 中心Y。
	const char* sPosZColumn_ = "z"; //! 中心Z。
	const char* sShapeColumn_ = "shape"; //! 湧き方の形。
	const char* sRadiusColumn_ = "radius"; //! 半径。
	const char* sIntervalColumn_ = "interval"; //! 湧く間隔。
	const char* sAmountColumn_ = "amount"; //! 1回に湧かせる数。
	const char* sTotalColumn_ = "total"; //! 合計の上限。

	/**
	 * @struct EnemyTypeName
	 * @brief  TSVに書く種類名と、ゾンビの種類の対応。
	 */
	struct EnemyTypeName
	{
		const char* pName_; //! TSVに書く種類名。
		nsApp::nsActor::EnEnemyType enEnemyType_; //! ゾンビの種類。
	};

	/* 種類を増やすときは、ここに1行足す。*/
	const EnemyTypeName aEnemyTypeNames_[] =
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
		for (const EnemyTypeName& entry : aEnemyTypeNames_)
		{
			if (typeName != entry.pName_)
				continue;

			outEnemyType = entry.enEnemyType_;
			return true;
		}

		return false;
	}

	/**
	 * @brief TSVに書いた形の名前を湧き方の形へ変換する。空欄は Point として扱う。
	 * @param shapeName 形の名前。
	 * @param outShape 変換した形。
	 * @return 知っている名前ならtrue。
	 */
	bool ToSpawnShape(const std::string& shapeName, nsApp::nsDirector::EnSpawnShape& outShape)
	{
		if (shapeName.empty() || shapeName == "Point")
		{
			outShape = nsApp::nsDirector::EnSpawnShape::Point;
			return true;
		}
		if (shapeName == "Circle")
		{
			outShape = nsApp::nsDirector::EnSpawnShape::Circle;
			return true;
		}
		if (shapeName == "Ring")
		{
			outShape = nsApp::nsDirector::EnSpawnShape::Ring;
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
			/* 種類名・形が分からない行は使わない(打ち間違いで意図しない湧き方をさせない)。*/
			if (!ToEnemyType(table.GetString(row, sTypeColumn_), outSetting.enEnemyType_))
				return false;
			if (!ToSpawnShape(table.GetString(row, sShapeColumn_), outSetting.enShape_))
				return false;

			/* 中心と湧き方を読む。空欄は既定値。*/
			outSetting.vCenter_.Set
			(
				table.GetFloat(row, sPosXColumn_),
				table.GetFloat(row, sPosYColumn_),
				table.GetFloat(row, sPosZColumn_)
			);

			/* 半径・間隔・数を読む。空欄は既定値。*/
			outSetting.fRadius_ = table.GetFloat(row, sRadiusColumn_, 0.0f);
			outSetting.fInterval_ = table.GetFloat(row, sIntervalColumn_, 0.0f);
			outSetting.iAmount_ = table.GetInt(row, sAmountColumn_, 1);
			outSetting.iTotal_ = table.GetInt(row, sTotalColumn_, 0);

			/* 1回に湧かせる数が0以下、半径が負の行は使わない。*/
			return outSetting.iAmount_ > 0 && outSetting.fRadius_ >= 0.0f;
		}
	}
}