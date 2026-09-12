#include "stdafx.h"
#include "Src/Data/WeaponStatusTable.h"
#include "Src/Data/ParameterFile.h"

namespace
{
	const char* sWeaponStatusFilePath_ = "Assets/data/weapon.json";	//! 武器ステータス表のファイルパス。
	const char* sWeaponsNodeName_ = "weapons";						//! 武器一覧が入っているノード名。

	/* 武器の種類に対応するJSONのキー名。EnWeaponTypeの並び順と一致させること。*/
	const char* sWeaponTypeNameList_[] =
	{
		"Handgun",		//! ハンドガン。
		"Magnum",		//! マグナム。
		"SMG",			//! サブマシンガン。
		"PumpShotgun",	//! ポンプショットガン。
		"AssaultRifle",	//! アサルトライフル。
		"AK47",			//! AK-47。
		"CombatRifle",	//! コンバットライフル。
		"HuntingRifle",	//! ハンティングライフル。
	};

	/* キー名の数がEnWeaponType::Numと一致しているかをコンパイル時に検査する。*/
	static_assert(
		_countof(sWeaponTypeNameList_) == static_cast<size_t>(nsApp::nsWeapon::EnWeaponType::Num),
		"sWeaponTypeNameList_ の要素数と EnWeaponType::Num が一致していません。");

	/* 引き金の挙動の名前(JSONの "fireMode" に書く文字列)。EnFireModeの並び順と一致させること。*/
	const char* sFireModeNameList_[] =
	{
		"Semi",		//! 単発。
		"Auto",		//! 連射。
		"Burst",	//! 点射。
	};

	/* 区分の名前(JSONの "slot" に書く文字列)。EnWeaponSlotの並び順と一致させること。*/
	const char* sSlotNameList_[] =
	{
		"Main",		//! メイン武器。
		"Sub",		//! サブ武器。
	};

	/**
	 * @struct WeaponStatusEntry
	 * @brief  武器1挺ぶんの保管場所。
	 *         WeaponStatus は武器名・モデルパス・リコイルパターンをポインタで指しているので、
	 *         その実体をここで持ち続けないと壊れたポインタになってしまう。
	 */
	struct WeaponStatusEntry
	{
		nsApp::nsWeapon::WeaponStatus stStatus_ = {};					//! 外へ渡すパラメータ。
		std::string sName_;										//! 武器名の実体。
		std::string sModelPath_;								//! モデルパスの実体。
		std::vector<nsApp::nsWeapon::RecoilStep> vecRecoilPattern_;	//! リコイルパターンの実体。
	};

	/* 全武器ぶんの保管場所。要素数が変わらないので、各要素のアドレスは動かない。*/
	WeaponStatusEntry aEntryList_[static_cast<size_t>(nsApp::nsWeapon::EnWeaponType::Num)];

	bool bIsLoaded_ = false;	//! JSONの読み込みを済ませたか。

	/**
	 * @struct WeaponDefault
	 * @brief  武器ごとに違う主要な既定値。細かい値(反動の戻りや持ち替え時間など)は全武器共通の既定値を使う。
	 * @note   調整はJSON(weapon.json)で行う。ここに書いてあるのは調整値ではなく、
	 *         JSONが読めない/項目が欠けているときに使うフォールバック。
	 *         アセットが揃っていない環境でも必ず起動できるようにするために置いている。
	 *         主要な値はJSON側と同じにしておくこと。
	 */
	struct WeaponDefault
	{
		const char* pName_;						//! 武器名。
		const char* pModelPath_;				//! モデルのパス。
		nsApp::nsWeapon::EnWeaponSlot enSlot_;	//! 区分。
		nsApp::nsWeapon::EnFireMode enFireMode_;//! 引き金の挙動。
		float fFireInterval_;					//! 発射間隔(秒)。
		int iMaxAmmo_;							//! マガジン弾数。
		int iMaxReserveAmmo_;					//! 予備弾の上限。
		float fReloadTime_;						//! リロード時間(秒)。1発ずつ装填なら1発ぶん。
		int iAttackPower_;						//! 1発の威力。
		int iPelletCount_;						//! 1発で飛ぶ弾の数。
		int iPenetrateCount_;					//! 貫通数。
		bool bIsShellReload_;					//! 1発ずつ装填するか。
		float fHandLength_;						//! 手に持たせたときの長さ。
		float fRecoilPitch_;					//! 1発の跳ね上がり(ラジアン)。
		float fSpreadHip_;						//! 腰だめの拡散(ラジアン)。
		float fSpreadAds_;						//! 覗き込みの拡散(ラジアン)。
		float fAdsZoomRate_;					//! 覗き込みの画角倍率。
	};

	/* 武器ごとの主要な既定値。EnWeaponTypeの並び順と一致させること。*/
	const WeaponDefault WEAPON_DEFAULT_TABLE[] =
	{
		/*  名前              モデル                                              区分   挙動   間隔    弾  予備  装填  威力 粒 貫通 1発ずつ 長さ   跳ね    腰      ADS     倍率 */
		{ "Handgun",      "Assets/modelData/gun/subWeapon/m1911.tkm",      nsApp::nsWeapon::EnWeaponSlot::Sub,  nsApp::nsWeapon::EnFireMode::Semi,  0.25f, 15,   0, 2.2f, 18,  1,  0, false, 22.0f, 0.030f, 0.035f, 0.004f, 0.80f },
		{ "Magnum",       "Assets/modelData/gun/subWeapon/mp r8.tkm",      nsApp::nsWeapon::EnWeaponSlot::Sub,  nsApp::nsWeapon::EnFireMode::Semi,  0.40f,  8,   0, 2.6f, 40,  1,  1, false, 26.0f, 0.060f, 0.030f, 0.003f, 0.75f },
		{ "SMG",          "Assets/modelData/gun/subWeapon/tec-9.tkm",      nsApp::nsWeapon::EnWeaponSlot::Main, nsApp::nsWeapon::EnFireMode::Auto,  0.06f, 50, 325, 2.2f, 12,  1,  0, false, 40.0f, 0.008f, 0.060f, 0.012f, 0.80f },
		{ "PumpShotgun",  "Assets/modelData/gun/mainWeapon/HAMR.tkm",      nsApp::nsWeapon::EnWeaponSlot::Main, nsApp::nsWeapon::EnFireMode::Semi,  0.90f,  8,  56, 0.55f, 15, 10,  0, true,  95.0f, 0.070f, 0.090f, 0.070f, 0.90f },
		{ "AssaultRifle", "Assets/modelData/gun/mainWeapon/M4A1.tkm",      nsApp::nsWeapon::EnWeaponSlot::Main, nsApp::nsWeapon::EnFireMode::Auto,  0.10f, 30, 180, 2.8f, 20,  1,  0, false, 85.0f, 0.014f, 0.050f, 0.008f, 0.70f },
		{ "AK47",         "Assets/modelData/gun/mainWeapon/AK-47.tkm",     nsApp::nsWeapon::EnWeaponSlot::Main, nsApp::nsWeapon::EnFireMode::Auto,  0.14f, 40, 180, 3.0f, 35,  1,  0, false, 87.0f, 0.022f, 0.065f, 0.012f, 0.70f },
		{ "CombatRifle",  "Assets/modelData/gun/mainWeapon/SCAR.tkm",      nsApp::nsWeapon::EnWeaponSlot::Main, nsApp::nsWeapon::EnFireMode::Burst, 0.30f, 60, 180, 2.8f, 26,  1,  0, false, 78.0f, 0.012f, 0.040f, 0.006f, 0.70f },
		{ "HuntingRifle", "Assets/modelData/gun/mainWeapon/L86A2.tkm",     nsApp::nsWeapon::EnWeaponSlot::Main, nsApp::nsWeapon::EnFireMode::Semi,  0.25f, 15, 150, 3.2f, 55,  1, 99, false, 95.0f, 0.050f, 0.045f, 0.001f, 0.35f },
	};

	/* 表の行数がEnWeaponType::Numと一致しているかをコンパイル時に検査する。*/
	static_assert(
		_countof(WEAPON_DEFAULT_TABLE) == static_cast<size_t>(nsApp::nsWeapon::EnWeaponType::Num),
		"WEAPON_DEFAULT_TABLE の行数と EnWeaponType::Num が一致していません。");

	/* 全武器共通の既定値(武器ごとの差はJSONで付ける)。*/
	const int iDefaultBurstCount_ = 3;				//! 点射の発数。
	const float fDefaultBurstInterval_ = 0.07f;		//! 点射の中の間隔(秒)。
	const float fDefaultDeployTime_ = 0.5f;			//! 持ち替え時間(秒)。
	const float fDefaultModelScale_ = 1.0f;			//! ビューモデルの倍率。
	const float fDefaultViewModelForward_ = 45.0f;	//! ビューモデルの前方距離。
	const float fDefaultHandOffset_ = 0.0f;			//! 手のボーンからのずれ(前・右・上)。
	const float fDefaultRecoilYaw_ = 0.008f;		//! 左右のブレ(ラジアン)。
	const float fDefaultKickBack_ = 8.0f;			//! キックバック距離。
	const float fDefaultSpreadPerShot_ = 0.005f;	//! 1発ごとに増える拡散(ラジアン)。
	const float fDefaultMaxSpreadShot_ = 0.060f;	//! 連射で増える拡散の上限(ラジアン)。
	const float fDefaultRecoilResetTime_ = 0.35f;	//! パターンが戻るまでの時間(秒)。
	const float fDefaultAdsSpeedRate_ = 0.65f;		//! 覗き込み中の移動速度倍率。
	const float fDefaultFalloffStart_ = 0.0f;		//! 威力が落ち始める距離(0で落ちない)。
	const float fDefaultFalloffEnd_ = 0.0f;			//! 威力が下限になる距離。
	const float fDefaultFalloffMinRate_ = 1.0f;		//! 威力の下限倍率。

	/* 全武器共通のリコイルパターン。素直に真上へ跳ね、撃ち続けると少しずつ左右へ散る。*/
	const nsApp::nsWeapon::RecoilStep DEFAULT_RECOIL_PATTERN[] =
	{
		{ 1.00f,  0.00f },
		{ 1.00f,  0.20f },
		{ 0.95f, -0.30f },
		{ 0.90f,  0.45f },
		{ 0.85f, -0.50f },
		{ 0.80f,  0.35f },
	};


	/**
	 * @brief 保管場所の文字列・配列を WeaponStatus のポインタへ結び直す。
	 *        文字列や配列を書き換えるとアドレスが変わることがあるので、書き換えた後に必ず呼ぶ。
	 * @param stEntry 結び直す武器。
	 */
	void LinkEntryPointers(WeaponStatusEntry& stEntry)
	{
		stEntry.stStatus_.pName_ = stEntry.sName_.c_str();
		stEntry.stStatus_.pModelPath_ = stEntry.sModelPath_.c_str();
		stEntry.stStatus_.pRecoilPattern_ = stEntry.vecRecoilPattern_.empty() ? nullptr : stEntry.vecRecoilPattern_.data();
		stEntry.stStatus_.iRecoilPatternCount_ = static_cast<int>(stEntry.vecRecoilPattern_.size());
	}


	/**
	 * @brief 既定値の表の1行から、武器1挺ぶんの既定値を組み立てる。
	 * @param stDefault 表の1行。
	 * @param stEntry   組み立て先。
	 */
	void BuildDefault(const WeaponDefault& stDefault, WeaponStatusEntry& stEntry)
	{
		nsApp::nsWeapon::WeaponStatus& stStatus = stEntry.stStatus_;

		stEntry.sName_ = stDefault.pName_;
		stEntry.sModelPath_ = stDefault.pModelPath_;

		/* 武器ごとに違う主要な値。*/
		stStatus.enSlot_ = stDefault.enSlot_;
		stStatus.enFireMode_ = stDefault.enFireMode_;
		stStatus.fFireInterval_ = stDefault.fFireInterval_;
		stStatus.iMaxAmmo_ = stDefault.iMaxAmmo_;
		stStatus.iMaxReserveAmmo_ = stDefault.iMaxReserveAmmo_;
		stStatus.fReloadTime_ = stDefault.fReloadTime_;
		stStatus.iAttackPower_ = stDefault.iAttackPower_;
		stStatus.iPelletCount_ = stDefault.iPelletCount_;
		stStatus.iPenetrateCount_ = stDefault.iPenetrateCount_;
		stStatus.bIsShellReload_ = stDefault.bIsShellReload_;
		stStatus.fHandLength_ = stDefault.fHandLength_;
		stStatus.fRecoilPitch_ = stDefault.fRecoilPitch_;
		stStatus.fSpreadHip_ = stDefault.fSpreadHip_;
		stStatus.fSpreadAds_ = stDefault.fSpreadAds_;
		stStatus.fAdsZoomRate_ = stDefault.fAdsZoomRate_;

		/* 全武器共通の値。*/
		stStatus.iBurstCount_ = iDefaultBurstCount_;
		stStatus.fBurstInterval_ = fDefaultBurstInterval_;
		stStatus.fDeployTime_ = fDefaultDeployTime_;
		stStatus.fModelScale_ = fDefaultModelScale_;
		stStatus.fViewModelForward_ = fDefaultViewModelForward_;
		stStatus.fHandForward_ = stDefault.fHandLength_ * 0.5f;	/* 銃はモデルの中心が手に来るので、長さの半分ずらすと握りが合う。*/
		stStatus.fHandRight_ = fDefaultHandOffset_;
		stStatus.fHandUp_ = fDefaultHandOffset_;
		stStatus.fRecoilYaw_ = fDefaultRecoilYaw_;
		stStatus.fKickBack_ = fDefaultKickBack_;
		stStatus.fSpreadPerShot_ = fDefaultSpreadPerShot_;
		stStatus.fMaxSpreadShot_ = fDefaultMaxSpreadShot_;
		stStatus.fRecoilResetTime_ = fDefaultRecoilResetTime_;
		stStatus.fAdsSpeedRate_ = fDefaultAdsSpeedRate_;
		stStatus.fFalloffStart_ = fDefaultFalloffStart_;
		stStatus.fFalloffEnd_ = fDefaultFalloffEnd_;
		stStatus.fFalloffMinRate_ = fDefaultFalloffMinRate_;

		stEntry.vecRecoilPattern_.assign(DEFAULT_RECOIL_PATTERN, DEFAULT_RECOIL_PATTERN + _countof(DEFAULT_RECOIL_PATTERN));
	}


	/**
	 * @brief 全武器を既定値で埋める。
	 */
	void BuildDefaults()
	{
		const size_t iCount = static_cast<size_t>(nsApp::nsWeapon::EnWeaponType::Num);
		for (size_t i = 0; i < iCount; i++)
		{
			BuildDefault(WEAPON_DEFAULT_TABLE[i], aEntryList_[i]);

			/* 文字列と配列の実体が決まったので、ポインタを結び直す。*/
			LinkEntryPointers(aEntryList_[i]);
		}
	}


	/**
	 * @brief 名前の一覧から、文字列に一致する番号を探す。
	 * @param pNameList 名前の一覧。
	 * @param iCount    一覧の数。
	 * @param sName     探す文字列。
	 * @param iDefault  見つからなかったときの番号。
	 * @return 一致した番号。
	 */
	int FindNameIndex(const char* const* pNameList, int iCount, const std::string& sName, int iDefault)
	{
		for (int i = 0; i < iCount; i++)
		{
			if (sName == pNameList[i])
				return i;
		}

		return iDefault;
	}


	/**
	 * @brief JSONの1武器ぶんの内容で既定値を上書きする。
	 *        項目が無ければ既定値をそのまま渡すので、書かれた項目だけが差し替わる。
	 * @param stFile  読み込み位置を対象の武器に合わせたJSONファイル。
	 * @param stEntry 上書きする武器。
	 */
	void OverwriteFromFile(const nsApp::nsData::ParameterFile& stFile, WeaponStatusEntry& stEntry)
	{
		nsApp::nsWeapon::WeaponStatus& stStatus = stEntry.stStatus_;

		stEntry.sName_ = stFile.GetString("name", stEntry.sName_.c_str());
		stEntry.sModelPath_ = stFile.GetString("modelPath", stEntry.sModelPath_.c_str());

		/* 区分と引き金の挙動は文字列で書く。一覧に無い文字列なら既定値のままにする。*/
		const int iSlot = static_cast<int>(stStatus.enSlot_);
		const std::string sSlot = stFile.GetString("slot", sSlotNameList_[iSlot]);
		stStatus.enSlot_ = static_cast<nsApp::nsWeapon::EnWeaponSlot>(FindNameIndex(sSlotNameList_, _countof(sSlotNameList_), sSlot, iSlot));

		const int iFireMode = static_cast<int>(stStatus.enFireMode_);
		const std::string sFireMode = stFile.GetString("fireMode", sFireModeNameList_[iFireMode]);
		stStatus.enFireMode_ = static_cast<nsApp::nsWeapon::EnFireMode>(FindNameIndex(sFireModeNameList_, _countof(sFireModeNameList_), sFireMode, iFireMode));

		stStatus.fFireInterval_ = stFile.GetFloat("fireInterval", stStatus.fFireInterval_);
		stStatus.iBurstCount_ = stFile.GetInt("burstCount", stStatus.iBurstCount_);
		stStatus.fBurstInterval_ = stFile.GetFloat("burstInterval", stStatus.fBurstInterval_);
		stStatus.iMaxAmmo_ = stFile.GetInt("maxAmmo", stStatus.iMaxAmmo_);
		stStatus.iMaxReserveAmmo_ = stFile.GetInt("maxReserveAmmo", stStatus.iMaxReserveAmmo_);
		stStatus.fReloadTime_ = stFile.GetFloat("reloadTime", stStatus.fReloadTime_);
		stStatus.bIsShellReload_ = stFile.GetBool("isShellReload", stStatus.bIsShellReload_);
		stStatus.fDeployTime_ = stFile.GetFloat("deployTime", stStatus.fDeployTime_);
		stStatus.iAttackPower_ = stFile.GetInt("attackPower", stStatus.iAttackPower_);
		stStatus.iPelletCount_ = stFile.GetInt("pelletCount", stStatus.iPelletCount_);
		stStatus.iPenetrateCount_ = stFile.GetInt("penetrateCount", stStatus.iPenetrateCount_);
		stStatus.fFalloffStart_ = stFile.GetFloat("falloffStart", stStatus.fFalloffStart_);
		stStatus.fFalloffEnd_ = stFile.GetFloat("falloffEnd", stStatus.fFalloffEnd_);
		stStatus.fFalloffMinRate_ = stFile.GetFloat("falloffMinRate", stStatus.fFalloffMinRate_);
		stStatus.fModelScale_ = stFile.GetFloat("modelScale", stStatus.fModelScale_);
		stStatus.fViewModelForward_ = stFile.GetFloat("viewModelForward", stStatus.fViewModelForward_);
		stStatus.fHandForward_ = stFile.GetFloat("handForward", stStatus.fHandForward_);
		stStatus.fHandRight_ = stFile.GetFloat("handRight", stStatus.fHandRight_);
		stStatus.fHandUp_ = stFile.GetFloat("handUp", stStatus.fHandUp_);
		stStatus.fHandLength_ = stFile.GetFloat("handLength", stStatus.fHandLength_);
		stStatus.fRecoilPitch_ = stFile.GetFloat("recoilPitch", stStatus.fRecoilPitch_);
		stStatus.fRecoilYaw_ = stFile.GetFloat("recoilYaw", stStatus.fRecoilYaw_);
		stStatus.fKickBack_ = stFile.GetFloat("kickBack", stStatus.fKickBack_);
		stStatus.fSpreadHip_ = stFile.GetFloat("spreadHip", stStatus.fSpreadHip_);
		stStatus.fSpreadAds_ = stFile.GetFloat("spreadAds", stStatus.fSpreadAds_);
		stStatus.fSpreadPerShot_ = stFile.GetFloat("spreadPerShot", stStatus.fSpreadPerShot_);
		stStatus.fMaxSpreadShot_ = stFile.GetFloat("maxSpreadShot", stStatus.fMaxSpreadShot_);
		stStatus.fRecoilResetTime_ = stFile.GetFloat("recoilResetTime", stStatus.fRecoilResetTime_);
		stStatus.fAdsZoomRate_ = stFile.GetFloat("adsZoomRate", stStatus.fAdsZoomRate_);
		stStatus.fAdsSpeedRate_ = stFile.GetFloat("adsSpeedRate", stStatus.fAdsSpeedRate_);

		/* リコイルパターンは [上への倍率, 左右への倍率] の並び。書かれていれば丸ごと差し替える。*/
		std::vector<float> vecPitchList;
		std::vector<float> vecYawList;
		if (stFile.GetFloatPairArray("recoilPattern", vecPitchList, vecYawList))
		{
			stEntry.vecRecoilPattern_.clear();

			const size_t iCount = vecPitchList.size();
			for (size_t i = 0; i < iCount; i++)
				stEntry.vecRecoilPattern_.push_back({ vecPitchList[i], vecYawList[i] });
		}

		/* 文字列と配列を書き換えたので、ポインタを結び直す。*/
		LinkEntryPointers(stEntry);
	}
}


namespace nsApp
{
	namespace nsData
	{
		void WeaponStatusTable::Load()
		{
			/* まず既定値で埋める。JSONが無くても、この値でそのまま遊べる。*/
			BuildDefaults();
			bIsLoaded_ = true;

			/* JSONを開く。無ければ既定値のままにする。*/
			ParameterFile stFile;
			if (!stFile.Load(sWeaponStatusFilePath_))
			{
				DebugPrintW(L"[WeaponStatusTable] weapon.json が読めないので既定値を使います。\n");
				return;
			}

			/* 武器一覧のノードへ潜る。*/
			if (!stFile.Enter(sWeaponsNodeName_))
			{
				DebugPrintW(L"[WeaponStatusTable] weapon.json に武器一覧が無いので既定値を使います。\n");
				return;
			}

			/* 武器の種類ごとに、名前が一致するノードの内容で既定値を上書きする。*/
			const int iNumType = static_cast<int>(nsWeapon::EnWeaponType::Num);
			for (int i = 0; i < iNumType; i++)
			{
				/* 一覧の位置から目的の武器へ潜り、読み終えたら一覧の位置へ戻す。*/
				if (!stFile.Enter(sWeaponTypeNameList_[i]))
					continue;

				OverwriteFromFile(stFile, aEntryList_[i]);
				stFile.Leave();
			}
		}


		const nsWeapon::WeaponStatus& WeaponStatusTable::Get(nsWeapon::EnWeaponType enType)
		{
			/* 初回の呼び出しでJSONを読み込む。*/
			if (!bIsLoaded_)
				Load();

			/* 範囲外を渡されたら先頭の武器を返す(落とさないための保険)。*/
			size_t iIndex = static_cast<size_t>(enType);
			if (iIndex >= static_cast<size_t>(nsWeapon::EnWeaponType::Num))
				iIndex = 0;

			return aEntryList_[iIndex].stStatus_;
		}


		const char* WeaponStatusTable::GetTypeName(nsWeapon::EnWeaponType enType)
		{
			/* 範囲外を渡されたら先頭の名前を返す。*/
			size_t iIndex = static_cast<size_t>(enType);
			if (iIndex >= static_cast<size_t>(nsWeapon::EnWeaponType::Num))
				iIndex = 0;

			return sWeaponTypeNameList_[iIndex];
		}


		bool WeaponStatusTable::FindTypeByName(const char* pName, nsWeapon::EnWeaponType& enOutType)
		{
			/* 名前の一覧から探す。無ければ -1 が返る。*/
			const int iIndex = FindNameIndex(sWeaponTypeNameList_, _countof(sWeaponTypeNameList_), pName, -1);
			if (iIndex < 0)
				return false;

			enOutType = static_cast<nsWeapon::EnWeaponType>(iIndex);
			return true;
		}
	}
}
