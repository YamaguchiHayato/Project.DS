#include "stdafx.h"
#include "Src/Item/PickupSpawner.h"
#include "Src/Item/Pickup.h"
#include "Src/Data/WeaponStatusTable.h"

namespace
{
	const float fTwoPi_ = 6.2831853f;	//! 一周の角度(ラジアン)。

	/**
	 * @struct PickupTypeName
	 * @brief  配置表に書く物資の名前と、種類の対応。
	 */
	struct PickupTypeName
	{
		const char* pName_;							//! 表に書く名前。
		nsApp::nsItem::EnPickupType enType_;		//! 対応する種類。
	};

	/* 銃以外の物資の名前。銃の名前は weapon.json のキー名をそのまま使うので、ここには並べない。*/
	const PickupTypeName PICKUP_TYPE_NAME_TABLE[] =
	{
		{ "Ammo",		nsApp::nsItem::EnPickupType::Ammo },
		{ "Medkit",		nsApp::nsItem::EnPickupType::Medkit },
		{ "PipeBomb",	nsApp::nsItem::EnPickupType::PipeBomb },
		{ "Pills",		nsApp::nsItem::EnPickupType::Pills },
		{ "Adrenaline",	nsApp::nsItem::EnPickupType::Adrenaline },
	};

	/**
	 * @brief 0〜1の乱数を返す。
	 * @return 0〜1の乱数。
	 */
	float Random01()
	{
		return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

	/**
	 * @brief 配置表に書かれた名前を、銃以外の物資の種類へ変換する。
	 * @param sName     表に書かれた名前。
	 * @param enOutType 変換した種類。
	 * @return 知っている名前なら true。
	 */
	bool ToPickupType(const std::string& sName, nsApp::nsItem::EnPickupType& enOutType)
	{
		for (const PickupTypeName& stEntry : PICKUP_TYPE_NAME_TABLE)
		{
			if (sName != stEntry.pName_)
				continue;

			enOutType = stEntry.enType_;
			return true;
		}

		return false;
	}
}

namespace nsApp
{
	namespace nsItem
	{
		bool PickupSpawner::Spawn(const char* pFilePath)
		{
			/* 配置表を読む。読めなければ物資は置かない(ステージ自体は遊べるようにしておく)。*/
			std::vector<PickupSpawnSetting> vecSettings;
			if (!nsK2EngineLow::nsSystem::nsTSVFile::TSVTableLoader::LoadList(pFilePath, vecSettings))
				return false;

			/* 1行につき1地点ぶんの抽選をする。*/
			bool bPlaced = false;
			for (const PickupSpawnSetting& stSetting : vecSettings)
			{
				if (PlaceOne(stSetting))
					bPlaced = true;
			}

			return bPlaced;
		}


		bool PickupSpawner::PlaceOne(const PickupSpawnSetting& stSetting)
		{
			/* その地点に物資が出るかを決める。外れたら何も置かない。*/
			if (Random01() > stSetting.fChance_)
				return false;

			/* 候補から1つ選ぶ。同じ名前を2回書けば、そのぶん出やすくなる。*/
			const int iCandidateCount = static_cast<int>(stSetting.vecCandidates_.size());
			const int iIndex = static_cast<int>(Random01() * static_cast<float>(iCandidateCount));
			const std::string& sName = stSetting.vecCandidates_[(iIndex < iCandidateCount) ? iIndex : (iCandidateCount - 1)];

			const Vector3 vPosition = MakeScatteredPosition(stSetting);

			/* まず銃の名前として探す。weapon.json のキー名がそのまま使える。*/
			nsWeapon::EnWeaponType enWeaponType = nsWeapon::EnWeaponType::Handgun;
			if (nsData::WeaponStatusTable::FindTypeByName(sName.c_str(), enWeaponType))
			{
				NewGO<Pickup>(0, "pickup")->SetupWeapon(enWeaponType, vPosition);
				return true;
			}

			/* 銃でなければ、それ以外の物資として探す。*/
			EnPickupType enPickupType = EnPickupType::Ammo;
			if (ToPickupType(sName, enPickupType))
			{
				NewGO<Pickup>(0, "pickup")->Setup(enPickupType, vPosition);
				return true;
			}

			/*
			 * どちらの名前でもなかった。打ち間違いか、まだ実装していない物資。
			 * 勝手に別のものを置くと気づけないので、何も置かずに知らせるだけにする。
			 */
			DebugPrintW(L"[PickupSpawner] 配置表に知らない名前があります: %hs\n", sName.c_str());
			return false;
		}


		Vector3 PickupSpawner::MakeScatteredPosition(const PickupSpawnSetting& stSetting) const
		{
			/* ばらつかせないなら中心をそのまま使う。*/
			if (stSetting.fRadius_ <= 0.0f)
				return stSetting.vCenter_;

			/*
			 * 円の中に偏りなく散らす。
			 * 半径に平方根を掛けないと中心へ寄ってしまう(EnemySpawner と同じ作法)。
			 */
			const float fAngle = Random01() * fTwoPi_;
			const float fLength = sqrtf(Random01()) * stSetting.fRadius_;

			Vector3 vPosition = stSetting.vCenter_;
			vPosition.x += cosf(fAngle) * fLength;
			vPosition.z += sinf(fAngle) * fLength;

			return vPosition;
		}
	}
}
