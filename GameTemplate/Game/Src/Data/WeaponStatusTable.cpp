#include "stdafx.h"
#include "Src/Data/WeaponStatusTable.h"
#include "Src/Data/ParameterFile.h"

namespace nsApp
{
	namespace nsData
	{
		namespace
		{
			const char* sWeaponStatusFilePath_ = "Assets/data/weapon.json";	//! 武器ステータス表のファイルパス。
			const char* sWeaponsNodeName_ = "weapons";						//! 武器一覧が入っているノード名。

			/* 武器の種類に対応するJSONのキー名。EnWeaponTypeの並び順と一致させること。*/
			const char* sWeaponTypeNameList_[] =
			{
				"Handgun",		//! ハンドガン。
				"AssaultRifle",	//! アサルトライフル。
			};

			/* キー名の数がEnWeaponType::Numと一致しているかをコンパイル時に検査する。*/
			static_assert(
				_countof(sWeaponTypeNameList_) == static_cast<size_t>(nsWeapon::EnWeaponType::Num),
				"sWeaponTypeNameList_ の要素数と EnWeaponType::Num が一致していません。");

			/**
			 * @struct WeaponStatusEntry
			 * @brief  武器1挺ぶんの保管場所。
			 *         WeaponStatus は武器名・モデルパス・リコイルパターンをポインタで指しているので、
			 *         その実体をここで持ち続けないと壊れたポインタになってしまう。
			 */
			struct WeaponStatusEntry
			{
				nsWeapon::WeaponStatus stStatus_ = {};					//! 外へ渡すパラメータ。
				std::string sName_;										//! 武器名の実体。
				std::string sModelPath_;								//! モデルパスの実体。
				std::vector<nsWeapon::RecoilStep> vecRecoilPattern_;	//! リコイルパターンの実体。
			};

			/* 全武器ぶんの保管場所。要素数が変わらないので、各要素のアドレスは動かない。*/
			WeaponStatusEntry aEntryList_[static_cast<size_t>(nsWeapon::EnWeaponType::Num)];

			bool bIsLoaded_ = false;	//! JSONの読み込みを済ませたか。


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
			 * @brief ハンドガンの既定値を組み立てる。JSONが無い/項目が足りないときはこの値が使われる。
			 * @param stEntry 組み立て先。
			 */
			void BuildHandgunDefault(WeaponStatusEntry& stEntry)
			{
				nsWeapon::WeaponStatus& stStatus = stEntry.stStatus_;

				stEntry.sName_ = "Handgun";
				stEntry.sModelPath_ = "Assets/modelData/gun/subWeapon/m1911.tkm";

				stStatus.enSlot_ = nsWeapon::EnWeaponSlot::Sub;
				stStatus.fFireInterval_ = 0.25f;
				stStatus.iMaxAmmo_ = 8;
				stStatus.iMaxReserveAmmo_ = 0;
				stStatus.fReloadTime_ = 2.2f;
				stStatus.fDeployTime_ = 0.35f;
				stStatus.iAttackPower_ = 18;
				stStatus.bIsFullAuto_ = false;
				stStatus.fModelScale_ = 1.0f;
				stStatus.fViewModelForward_ = 55.0f;
				stStatus.fHandForward_ = 7.0f;
				stStatus.fHandRight_ = 0.0f;
				stStatus.fHandUp_ = 0.0f;
				stStatus.fHandLength_ = 22.0f;
				stStatus.fRecoilPitch_ = 0.030f;
				stStatus.fRecoilYaw_ = 0.010f;
				stStatus.fKickBack_ = 10.0f;
				stStatus.fSpreadHip_ = 0.035f;
				stStatus.fSpreadAds_ = 0.004f;
				stStatus.fSpreadPerShot_ = 0.006f;
				stStatus.fMaxSpreadShot_ = 0.060f;
				stStatus.fRecoilResetTime_ = 0.35f;
				stStatus.fAdsZoomRate_ = 0.80f;
				stStatus.fAdsSpeedRate_ = 0.70f;

				/* 単発なので素直に真上へ跳ね、撃ち続けると少しずつ左右へ散る。*/
				stEntry.vecRecoilPattern_ =
				{
					{ 1.00f,  0.00f },
					{ 1.00f,  0.25f },
					{ 0.95f, -0.30f },
					{ 0.90f,  0.45f },
					{ 0.85f, -0.50f },
					{ 0.80f,  0.35f },
				};
			}


			/**
			 * @brief アサルトライフルの既定値を組み立てる。
			 * @param stEntry 組み立て先。
			 */
			void BuildAssaultRifleDefault(WeaponStatusEntry& stEntry)
			{
				nsWeapon::WeaponStatus& stStatus = stEntry.stStatus_;

				stEntry.sName_ = "AssaultRifle";
				stEntry.sModelPath_ = "Assets/modelData/gun/mainWeapon/M4A1.tkm";

				stStatus.enSlot_ = nsWeapon::EnWeaponSlot::Main;
				stStatus.fFireInterval_ = 0.10f;
				stStatus.iMaxAmmo_ = 30;
				stStatus.iMaxReserveAmmo_ = 180;
				stStatus.fReloadTime_ = 2.8f;
				stStatus.fDeployTime_ = 0.55f;
				stStatus.iAttackPower_ = 20;
				stStatus.bIsFullAuto_ = true;
				stStatus.fModelScale_ = 2.0f;
				stStatus.fViewModelForward_ = 35.0f;
				stStatus.fHandForward_ = 30.0f;
				stStatus.fHandRight_ = 0.0f;
				stStatus.fHandUp_ = 0.0f;
				stStatus.fHandLength_ = 85.0f;
				stStatus.fRecoilPitch_ = 0.014f;
				stStatus.fRecoilYaw_ = 0.008f;
				stStatus.fKickBack_ = 6.0f;
				stStatus.fSpreadHip_ = 0.050f;
				stStatus.fSpreadAds_ = 0.008f;
				stStatus.fSpreadPerShot_ = 0.004f;
				stStatus.fMaxSpreadShot_ = 0.060f;
				stStatus.fRecoilResetTime_ = 0.35f;
				stStatus.fAdsZoomRate_ = 0.70f;
				stStatus.fAdsSpeedRate_ = 0.60f;

				/*
				 * 最初の数発は真上へ強く跳ね、そのあと右へ流れてから左へ返す。
				 * この形を覚えて逆へ動かせば制御できる、という遊びを狙っている。
				 */
				stEntry.vecRecoilPattern_ =
				{
					{ 1.00f,  0.00f },
					{ 1.00f,  0.05f },
					{ 1.00f,  0.15f },
					{ 0.90f,  0.35f },
					{ 0.85f,  0.55f },
					{ 0.80f,  0.70f },
					{ 0.75f,  0.60f },
					{ 0.70f,  0.30f },
					{ 0.65f, -0.10f },
					{ 0.60f, -0.45f },
					{ 0.60f, -0.70f },
					{ 0.55f, -0.60f },
					{ 0.55f, -0.30f },
					{ 0.50f,  0.10f },
					{ 0.50f,  0.40f },
				};
			}


			/**
			 * @brief 全武器を既定値で埋める。
			 */
			void BuildDefaults()
			{
				BuildHandgunDefault(aEntryList_[static_cast<size_t>(nsWeapon::EnWeaponType::Handgun)]);
				BuildAssaultRifleDefault(aEntryList_[static_cast<size_t>(nsWeapon::EnWeaponType::AssaultRifle)]);

				/* 文字列と配列の実体が決まったので、ポインタを結び直す。*/
				for (WeaponStatusEntry& stEntry : aEntryList_)
					LinkEntryPointers(stEntry);
			}


			/**
			 * @brief JSONの1武器ぶんの内容で既定値を上書きする。
			 *        項目が無ければ既定値をそのまま渡すので、書かれた項目だけが差し替わる。
			 * @param stFile  読み込み位置を対象の武器に合わせたJSONファイル。
			 * @param stEntry 上書きする武器。
			 */
			void OverwriteFromFile(const ParameterFile& stFile, WeaponStatusEntry& stEntry)
			{
				nsWeapon::WeaponStatus& stStatus = stEntry.stStatus_;

				stEntry.sName_ = stFile.GetString("name", stEntry.sName_.c_str());
				stEntry.sModelPath_ = stFile.GetString("modelPath", stEntry.sModelPath_.c_str());

				/* 区分は "Main"/"Sub" の文字列で書く。それ以外が書かれていれば既定値のままにする。*/
				const std::string sSlot = stFile.GetString("slot", (stStatus.enSlot_ == nsWeapon::EnWeaponSlot::Main) ? "Main" : "Sub");
				if (sSlot == "Main")
					stStatus.enSlot_ = nsWeapon::EnWeaponSlot::Main;
				else if (sSlot == "Sub")
					stStatus.enSlot_ = nsWeapon::EnWeaponSlot::Sub;

				stStatus.fFireInterval_ = stFile.GetFloat("fireInterval", stStatus.fFireInterval_);
				stStatus.iMaxAmmo_ = stFile.GetInt("maxAmmo", stStatus.iMaxAmmo_);
				stStatus.iMaxReserveAmmo_ = stFile.GetInt("maxReserveAmmo", stStatus.iMaxReserveAmmo_);
				stStatus.fReloadTime_ = stFile.GetFloat("reloadTime", stStatus.fReloadTime_);
				stStatus.fDeployTime_ = stFile.GetFloat("deployTime", stStatus.fDeployTime_);
				stStatus.iAttackPower_ = stFile.GetInt("attackPower", stStatus.iAttackPower_);
				stStatus.bIsFullAuto_ = stFile.GetBool("isFullAuto", stStatus.bIsFullAuto_);
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
	}
}
