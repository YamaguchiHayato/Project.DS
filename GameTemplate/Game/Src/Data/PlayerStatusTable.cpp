#include "stdafx.h"
#include "Src/Data/PlayerStatusTable.h"
#include "Src/Data/ParameterFile.h"

namespace
{
	const char* sPlayerStatusFilePath_ = "Assets/data/player.json";	//! プレイヤーステータス表のファイルパス。
	const char* sViewShakeNodeName_ = "viewShake";					//! 揺れの調整値が入っているノード名。
	const char* sReloadNodeName_ = "reload";						//! リロード演出の調整値が入っているノード名。

	nsApp::nsData::PlayerStatus stPlayerStatus_;	//! 読み込んだプレイヤーのパラメータ。
	bool bIsLoaded_ = false;						//! JSONの読み込みを済ませたか。

	/**
	 * @struct ReadEntry
	 * @brief  JSONのキーと、その値を入れるメンバの対応。
	 *         項目ごとに1行ずつ読み込む代わりに、この表を並べてまとめて読む。
	 *         項目を増やすときは表に1行足すだけでよい。
	 * @tparam TOwner 値を持つ構造体。
	 * @tparam TValue 値の型。
	 */
	template<class TOwner, class TValue>
	struct ReadEntry
	{
		const char* pKey_;					//! JSONのキー。
		TValue TOwner::* pMember_;			//! 値を入れるメンバ。
	};

	/* プレイヤーの小数の項目。*/
	const ReadEntry<nsApp::nsData::PlayerStatus, float> PLAYER_FLOAT_TABLE[] =
	{
		{ "moveSpeed",			&nsApp::nsData::PlayerStatus::fMoveSpeed_ },
		{ "sprintRate",			&nsApp::nsData::PlayerStatus::fSprintRate_ },
		{ "eyeHeight",			&nsApp::nsData::PlayerStatus::fEyeHeight_ },
		{ "weaponRange",		&nsApp::nsData::PlayerStatus::fWeaponRange_ },
		{ "bodyModelScale",		&nsApp::nsData::PlayerStatus::fBodyModelScale_ },
		{ "bleedOutTime",		&nsApp::nsData::PlayerStatus::fBleedOutTime_ },
		{ "shoveRange",			&nsApp::nsData::PlayerStatus::fShoveRange_ },
		{ "shovePush",			&nsApp::nsData::PlayerStatus::fShovePush_ },
		{ "shoveFrontDot",		&nsApp::nsData::PlayerStatus::fShoveFrontDot_ },
		{ "shoveCooldownTime",	&nsApp::nsData::PlayerStatus::fShoveCooldownTime_ },
	};

	/* プレイヤーの整数の項目。*/
	const ReadEntry<nsApp::nsData::PlayerStatus, int> PLAYER_INT_TABLE[] =
	{
		{ "maxHP",			&nsApp::nsData::PlayerStatus::iMaxHP_ },
		{ "reviveHP",		&nsApp::nsData::PlayerStatus::iReviveHP_ },
		{ "medkitCount",	&nsApp::nsData::PlayerStatus::iMedkitCount_ },
		{ "grenadeCount",	&nsApp::nsData::PlayerStatus::iGrenadeCount_ },
	};

	/* 歩きと視点移動の揺れの項目。*/
	const ReadEntry<nsApp::nsData::ViewShakeStatus, float> VIEW_SHAKE_TABLE[] =
	{
		{ "swayGain",				&nsApp::nsData::ViewShakeStatus::fSwayGain_ },
		{ "swayMaxOffset",			&nsApp::nsData::ViewShakeStatus::fSwayMaxOffset_ },
		{ "swayRecoverRate",		&nsApp::nsData::ViewShakeStatus::fSwayRecoverRate_ },
		{ "swayRollRate",			&nsApp::nsData::ViewShakeStatus::fSwayRollRate_ },
		{ "bobWalkSpeed",			&nsApp::nsData::ViewShakeStatus::fBobWalkSpeed_ },
		{ "bobWalkAmp",				&nsApp::nsData::ViewShakeStatus::fBobWalkAmp_ },
		{ "bobSprintSpeed",			&nsApp::nsData::ViewShakeStatus::fBobSprintSpeed_ },
		{ "bobSprintAmp",			&nsApp::nsData::ViewShakeStatus::fBobSprintAmp_ },
		{ "bobPhaseLag",			&nsApp::nsData::ViewShakeStatus::fBobPhaseLag_ },
		{ "bobWeaponUpRate",		&nsApp::nsData::ViewShakeStatus::fBobWeaponUpRate_ },
		{ "bobWeightRate",			&nsApp::nsData::ViewShakeStatus::fBobWeightRate_ },
		{ "viewBobHeightWalk",		&nsApp::nsData::ViewShakeStatus::fViewBobHeightWalk_ },
		{ "viewBobHeightSprint",	&nsApp::nsData::ViewShakeStatus::fViewBobHeightSprint_ },
		{ "viewBobRollWalk",		&nsApp::nsData::ViewShakeStatus::fViewBobRollWalk_ },
		{ "viewBobRollSprint",		&nsApp::nsData::ViewShakeStatus::fViewBobRollSprint_ },
		{ "strafeRollAngle",		&nsApp::nsData::ViewShakeStatus::fStrafeRollAngle_ },
		{ "strafeFollowRate",		&nsApp::nsData::ViewShakeStatus::fStrafeFollowRate_ },
		{ "adsSuppressRate",		&nsApp::nsData::ViewShakeStatus::fAdsSuppressRate_ },
	};

	/* リロード演出の項目。*/
	const ReadEntry<nsApp::nsData::ReloadMotionStatus, float> RELOAD_MOTION_TABLE[] =
	{
		{ "lowerDown",		&nsApp::nsData::ReloadMotionStatus::fLowerDown_ },
		{ "lowerAngle",		&nsApp::nsData::ReloadMotionStatus::fLowerAngle_ },
		{ "pullBack",		&nsApp::nsData::ReloadMotionStatus::fPullBack_ },
		{ "pullRight",		&nsApp::nsData::ReloadMotionStatus::fPullRight_ },
		{ "rollAngle",		&nsApp::nsData::ReloadMotionStatus::fRollAngle_ },
		{ "insertUp",		&nsApp::nsData::ReloadMotionStatus::fInsertUp_ },
		{ "insertAngle",	&nsApp::nsData::ReloadMotionStatus::fInsertAngle_ },
		{ "settleUp",		&nsApp::nsData::ReloadMotionStatus::fSettleUp_ },
	};

	/**
	 * @brief 表に並べた小数の項目をまとめて読み込む。
	 *        項目が無ければ既定値をそのまま渡すので、書かれた項目だけが差し替わる。
	 * @param stFile   読み込み位置を合わせたJSONファイル。
	 * @param stOwner  値を入れる構造体。
	 * @param pTable   キーとメンバの対応表。
	 * @param iCount   対応表の行数。
	 */
	template<class TOwner>
	void ReadFloatTable(const nsApp::nsData::ParameterFile& stFile, TOwner& stOwner, const ReadEntry<TOwner, float>* pTable, int iCount)
	{
		for (int i = 0; i < iCount; i++)
			stOwner.*(pTable[i].pMember_) = stFile.GetFloat(pTable[i].pKey_, stOwner.*(pTable[i].pMember_));
	}

	/**
	 * @brief 表に並べた整数の項目をまとめて読み込む。
	 * @param stFile  読み込み位置を合わせたJSONファイル。
	 * @param stOwner 値を入れる構造体。
	 * @param pTable  キーとメンバの対応表。
	 * @param iCount  対応表の行数。
	 */
	template<class TOwner>
	void ReadIntTable(const nsApp::nsData::ParameterFile& stFile, TOwner& stOwner, const ReadEntry<TOwner, int>* pTable, int iCount)
	{
		for (int i = 0; i < iCount; i++)
			stOwner.*(pTable[i].pMember_) = stFile.GetInt(pTable[i].pKey_, stOwner.*(pTable[i].pMember_));
	}
}


namespace nsApp
{
	namespace nsData
	{
		void PlayerStatusTable::Load()
		{
			/* まず既定値へ戻す。JSONが無くても、この値でそのまま遊べる。*/
			stPlayerStatus_ = PlayerStatus();
			bIsLoaded_ = true;

			/* JSONを開く。無ければ既定値のままにする。*/
			ParameterFile stFile;
			if (!stFile.Load(sPlayerStatusFilePath_))
			{
				DebugPrintW(L"[PlayerStatusTable] player.json が読めないので既定値を使います。\n");
				return;
			}

			/* 基本のステータスを表に従って読み込む。*/
			ReadFloatTable(stFile, stPlayerStatus_, PLAYER_FLOAT_TABLE, _countof(PLAYER_FLOAT_TABLE));
			ReadIntTable(stFile, stPlayerStatus_, PLAYER_INT_TABLE, _countof(PLAYER_INT_TABLE));

			/* 文字列はこれ1つなので、表にはせず直接読む。*/
			stPlayerStatus_.sHandBoneName_ = stFile.GetString("handBoneName", stPlayerStatus_.sHandBoneName_.c_str());

			/* 歩きと視点移動の揺れ。*/
			if (stFile.Enter(sViewShakeNodeName_))
			{
				ReadFloatTable(stFile, stPlayerStatus_.stViewShake_, VIEW_SHAKE_TABLE, _countof(VIEW_SHAKE_TABLE));
				stFile.Leave();
			}

			/* リロード演出の振れ幅。*/
			if (stFile.Enter(sReloadNodeName_))
			{
				ReadFloatTable(stFile, stPlayerStatus_.stReloadMotion_, RELOAD_MOTION_TABLE, _countof(RELOAD_MOTION_TABLE));
				stFile.Leave();
			}
		}


		const PlayerStatus& PlayerStatusTable::Get()
		{
			/* 初回の呼び出しでJSONを読み込む。*/
			if (!bIsLoaded_)
				Load();

			return stPlayerStatus_;
		}
	}
}
