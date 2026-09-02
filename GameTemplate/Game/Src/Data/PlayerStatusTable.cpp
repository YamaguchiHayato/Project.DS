#include "stdafx.h"
#include "Src/Data/PlayerStatusTable.h"
#include "Src/Data/ParameterFile.h"

namespace nsApp
{
	namespace nsData
	{
		namespace
		{
			const char* sPlayerStatusFilePath_ = "Assets/data/player.json";	//! プレイヤーステータス表のファイルパス。

			PlayerStatus stPlayerStatus_;	//! 読み込んだプレイヤーのパラメータ。
			bool bIsLoaded_ = false;		//! JSONの読み込みを済ませたか。
		}


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

			/* 項目が無ければ既定値をそのまま渡すので、書かれた項目だけが差し替わる。*/
			stPlayerStatus_.iMaxHP_ = stFile.GetInt("maxHP", stPlayerStatus_.iMaxHP_);
			stPlayerStatus_.fMoveSpeed_ = stFile.GetFloat("moveSpeed", stPlayerStatus_.fMoveSpeed_);
			stPlayerStatus_.fSprintRate_ = stFile.GetFloat("sprintRate", stPlayerStatus_.fSprintRate_);
			stPlayerStatus_.fEyeHeight_ = stFile.GetFloat("eyeHeight", stPlayerStatus_.fEyeHeight_);
			stPlayerStatus_.fWeaponRange_ = stFile.GetFloat("weaponRange", stPlayerStatus_.fWeaponRange_);
			stPlayerStatus_.fBodyModelScale_ = stFile.GetFloat("bodyModelScale", stPlayerStatus_.fBodyModelScale_);
			stPlayerStatus_.sHandBoneName_ = stFile.GetString("handBoneName", stPlayerStatus_.sHandBoneName_.c_str());

			stPlayerStatus_.fBleedOutTime_ = stFile.GetFloat("bleedOutTime", stPlayerStatus_.fBleedOutTime_);
			stPlayerStatus_.iReviveHP_ = stFile.GetInt("reviveHP", stPlayerStatus_.iReviveHP_);

			stPlayerStatus_.fShoveRange_ = stFile.GetFloat("shoveRange", stPlayerStatus_.fShoveRange_);
			stPlayerStatus_.fShovePush_ = stFile.GetFloat("shovePush", stPlayerStatus_.fShovePush_);
			stPlayerStatus_.fShoveFrontDot_ = stFile.GetFloat("shoveFrontDot", stPlayerStatus_.fShoveFrontDot_);
			stPlayerStatus_.fShoveCooldownTime_ = stFile.GetFloat("shoveCooldownTime", stPlayerStatus_.fShoveCooldownTime_);

			stPlayerStatus_.iMedkitCount_ = stFile.GetInt("medkitCount", stPlayerStatus_.iMedkitCount_);
			stPlayerStatus_.iGrenadeCount_ = stFile.GetInt("grenadeCount", stPlayerStatus_.iGrenadeCount_);

			/* 歩きと視点移動の揺れ。*/
			if (stFile.Enter("viewShake"))
			{
				stPlayerStatus_.stViewShake_.fSwayGain_ = stFile.GetFloat("swayGain", stPlayerStatus_.stViewShake_.fSwayGain_);
				stPlayerStatus_.stViewShake_.fSwayMaxOffset_ = stFile.GetFloat("swayMaxOffset", stPlayerStatus_.stViewShake_.fSwayMaxOffset_);
				stPlayerStatus_.stViewShake_.fSwayRecoverRate_ = stFile.GetFloat("swayRecoverRate", stPlayerStatus_.stViewShake_.fSwayRecoverRate_);
				stPlayerStatus_.stViewShake_.fSwayRollRate_ = stFile.GetFloat("swayRollRate", stPlayerStatus_.stViewShake_.fSwayRollRate_);
				stPlayerStatus_.stViewShake_.fBobWalkSpeed_ = stFile.GetFloat("bobWalkSpeed", stPlayerStatus_.stViewShake_.fBobWalkSpeed_);
				stPlayerStatus_.stViewShake_.fBobWalkAmp_ = stFile.GetFloat("bobWalkAmp", stPlayerStatus_.stViewShake_.fBobWalkAmp_);
				stPlayerStatus_.stViewShake_.fBobSprintSpeed_ = stFile.GetFloat("bobSprintSpeed", stPlayerStatus_.stViewShake_.fBobSprintSpeed_);
				stPlayerStatus_.stViewShake_.fBobSprintAmp_ = stFile.GetFloat("bobSprintAmp", stPlayerStatus_.stViewShake_.fBobSprintAmp_);
				stPlayerStatus_.stViewShake_.fBobPhaseLag_ = stFile.GetFloat("bobPhaseLag", stPlayerStatus_.stViewShake_.fBobPhaseLag_);
				stPlayerStatus_.stViewShake_.fBobWeaponUpRate_ = stFile.GetFloat("bobWeaponUpRate", stPlayerStatus_.stViewShake_.fBobWeaponUpRate_);
				stPlayerStatus_.stViewShake_.fBobWeightRate_ = stFile.GetFloat("bobWeightRate", stPlayerStatus_.stViewShake_.fBobWeightRate_);
				stPlayerStatus_.stViewShake_.fViewBobHeightWalk_ = stFile.GetFloat("viewBobHeightWalk", stPlayerStatus_.stViewShake_.fViewBobHeightWalk_);
				stPlayerStatus_.stViewShake_.fViewBobHeightSprint_ = stFile.GetFloat("viewBobHeightSprint", stPlayerStatus_.stViewShake_.fViewBobHeightSprint_);
				stPlayerStatus_.stViewShake_.fViewBobRollWalk_ = stFile.GetFloat("viewBobRollWalk", stPlayerStatus_.stViewShake_.fViewBobRollWalk_);
				stPlayerStatus_.stViewShake_.fViewBobRollSprint_ = stFile.GetFloat("viewBobRollSprint", stPlayerStatus_.stViewShake_.fViewBobRollSprint_);
				stPlayerStatus_.stViewShake_.fStrafeRollAngle_ = stFile.GetFloat("strafeRollAngle", stPlayerStatus_.stViewShake_.fStrafeRollAngle_);
				stPlayerStatus_.stViewShake_.fStrafeFollowRate_ = stFile.GetFloat("strafeFollowRate", stPlayerStatus_.stViewShake_.fStrafeFollowRate_);
				stPlayerStatus_.stViewShake_.fAdsSuppressRate_ = stFile.GetFloat("adsSuppressRate", stPlayerStatus_.stViewShake_.fAdsSuppressRate_);

				stFile.Leave();
			}

			/* リロード演出の振れ幅。*/
			if (stFile.Enter("reload"))
			{
				stPlayerStatus_.stReloadMotion_.fLowerDown_ = stFile.GetFloat("lowerDown", stPlayerStatus_.stReloadMotion_.fLowerDown_);
				stPlayerStatus_.stReloadMotion_.fLowerAngle_ = stFile.GetFloat("lowerAngle", stPlayerStatus_.stReloadMotion_.fLowerAngle_);
				stPlayerStatus_.stReloadMotion_.fPullBack_ = stFile.GetFloat("pullBack", stPlayerStatus_.stReloadMotion_.fPullBack_);
				stPlayerStatus_.stReloadMotion_.fPullRight_ = stFile.GetFloat("pullRight", stPlayerStatus_.stReloadMotion_.fPullRight_);
				stPlayerStatus_.stReloadMotion_.fRollAngle_ = stFile.GetFloat("rollAngle", stPlayerStatus_.stReloadMotion_.fRollAngle_);
				stPlayerStatus_.stReloadMotion_.fInsertUp_ = stFile.GetFloat("insertUp", stPlayerStatus_.stReloadMotion_.fInsertUp_);
				stPlayerStatus_.stReloadMotion_.fInsertAngle_ = stFile.GetFloat("insertAngle", stPlayerStatus_.stReloadMotion_.fInsertAngle_);
				stPlayerStatus_.stReloadMotion_.fSettleUp_ = stFile.GetFloat("settleUp", stPlayerStatus_.stReloadMotion_.fSettleUp_);

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
