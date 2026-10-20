#include "stdafx.h"
#include "EnemyStatus.h"

namespace
{
	const char* TYPE_COLUMN = "type";
	const char* MAX_HP_COLUMN = "maxHp";
	const char* AGGRO_RANGE_COLUMN = "aggroRange";
	const char* CHASE_SPEED_COLUMN = "chaseSpeed";
	const char* ATTACK_RANGE_COLUMN = "attackRange";
	const char* EXPLODE_RANGE_COLUMN = "explodeRange";
	const char* EXPLODE_DAMAGE_COLUMN = "explodeDamage";
}

namespace nsApp
{
	namespace nsData
	{
		bool EnemyStatus::Create(const nsK2EngineLow::nsSystem::nsTSVFile::TSVTable& table, int row, EnemyStatus& outStatus)
		{
			/* 種類名が空の行は使わない。変換は表側の LoadTable が行う。*/
			const std::string sType = table.GetString(row, TYPE_COLUMN);
			if (sType.empty())
				return false;

			outStatus.iMaxHp_ = table.GetInt(row, MAX_HP_COLUMN, outStatus.iMaxHp_);
			outStatus.fAggroRange_ = table.GetFloat(row, AGGRO_RANGE_COLUMN, outStatus.fAggroRange_);
			outStatus.fChaseSpeed_ = table.GetFloat(row, CHASE_SPEED_COLUMN, outStatus.fChaseSpeed_);
			outStatus.fAttackRange_ = table.GetFloat(row, ATTACK_RANGE_COLUMN, outStatus.fAttackRange_);
			outStatus.fExplodeRange_ = table.GetFloat(row, EXPLODE_RANGE_COLUMN, outStatus.fExplodeRange_);
			outStatus.iExplodeDamage_ = table.GetInt(row, EXPLODE_DAMAGE_COLUMN, outStatus.iExplodeDamage_);
			return outStatus.iMaxHp_ > 0;
		}
	}
}