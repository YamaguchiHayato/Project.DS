#include "stdafx.h"
#include "Src/Combat/HitBoxSet.h"
#include "Src/Data/ParameterFile.h"

namespace
{
	const char* sHitBoxFilePath_ = "Assets/data/hitbox.json";	//! 当たり判定表のファイルパス。
	const char* sHitBoxSetsNodeName_ = "hitBoxSets";			//! 当たり判定一覧が入っているノード名。
	const char* sDamageRateNodeName_ = "damageRate";			//! ダメージ倍率が入っているノード名。
	const char* sPartsArrayName_ = "parts";						//! 部位の形が並んでいる配列名。

	/* 部位に対応するJSONのキー名。EnHitPartの並び順と一致させること。*/
	const char* sPartNameList_[] =
	{
		"Head",	//! 頭。
		"Body",	//! 胴と腕。
		"Leg",	//! 脚。
	};

	/* キー名の数がEnHitPart::Numと一致しているかをコンパイル時に検査する。*/
	static_assert(
		_countof(sPartNameList_) == static_cast<size_t>(nsApp::nsCombat::EnHitPart::Num),
		"sPartNameList_ の要素数と EnHitPart::Num が一致していません。");

	/* キャラクター種別に対応するJSONのキー名。*/
	const char* sInfectedSetName_ = "Infected";	//! 雑魚ゾンビ。
	const char* sSurvivorSetName_ = "Survivor";	//! サバイバー。

	nsApp::nsCombat::HitBoxSet stInfectedHitBoxSet_;	//! 雑魚ゾンビの当たり判定。
	nsApp::nsCombat::HitBoxSet stSurvivorHitBoxSet_;	//! サバイバーの当たり判定。
	bool bIsLoaded_ = false;		//! JSONの読み込みを済ませたか。


	/**
	 * @brief 名前から部位を求める。
	 * @param sName 部位の名前。
	 * @return 対応する部位。見つからなければ EnHitPart::None。
	 */
	nsApp::nsCombat::EnHitPart FindPartByName(const std::string& sName)
	{
		const int iNumPart = static_cast<int>(nsApp::nsCombat::EnHitPart::Num);
		for (int i = 0; i < iNumPart; i++)
		{
			if (sName == sPartNameList_[i])
				return static_cast<nsApp::nsCombat::EnHitPart>(i);
		}

		return nsApp::nsCombat::EnHitPart::None;
	}


	/**
	 * @brief 人型の既定の当たり判定を組み立てる。JSONが無いときはこの形が使われる。
	 *        身長175前後のモデル(unityChan/ゾンビ)に合わせた寸法。
	 * @param stSet 組み立て先。
	 */
	void BuildHumanDefault(nsApp::nsCombat::HitBoxSet& stSet)
	{
		/* 積み直すので、前の形と倍率を捨てる。*/
		stSet.Clear();

		/* 上から順に、頭・胴(腕を含む)・脚。*/
		stSet.AddPartShape({ nsApp::nsCombat::EnHitPart::Head, 148.0f, 178.0f, 20.0f });
		stSet.AddPartShape({ nsApp::nsCombat::EnHitPart::Body,  75.0f, 148.0f, 38.0f });
		stSet.AddPartShape({ nsApp::nsCombat::EnHitPart::Leg,    0.0f,  75.0f, 28.0f });

		/* 部位ごとのダメージ倍率。頭だけ大きく、脚は効きを悪くする。*/
		stSet.SetDamageRate(nsApp::nsCombat::EnHitPart::Head, 2.0f);
		stSet.SetDamageRate(nsApp::nsCombat::EnHitPart::Body, 1.0f);
		stSet.SetDamageRate(nsApp::nsCombat::EnHitPart::Leg, 0.7f);
	}


	/**
	 * @brief JSONの1キャラクターぶんの内容で当たり判定を作り直す。
	 *        部位が1つも書かれていなければ、既定の形をそのまま残す。
	 * @param stFile 読み込み位置を対象のキャラクターに合わせたJSONファイル。
	 * @param stSet  作り直す当たり判定。
	 */
	void OverwriteFromFile(nsApp::nsData::ParameterFile& stFile, nsApp::nsCombat::HitBoxSet& stSet)
	{
		/* ダメージ倍率は書かれているものだけ差し替える。*/
		if (stFile.Enter(sDamageRateNodeName_))
		{
			const int iNumPart = static_cast<int>(nsApp::nsCombat::EnHitPart::Num);
			for (int i = 0; i < iNumPart; i++)
			{
				const nsApp::nsCombat::EnHitPart enPart = static_cast<nsApp::nsCombat::EnHitPart>(i);
				stSet.SetDamageRate(enPart, stFile.GetFloat(sPartNameList_[i], stSet.GetDamageRate(enPart)));
			}

			stFile.Leave();
		}

		/* 部位の形は、書かれていれば丸ごと差し替える。*/
		const int iNumShape = stFile.GetArraySize(sPartsArrayName_);
		if (iNumShape <= 0)
			return;

		/* 倍率は残したいので、形だけを組み直す。*/
		std::vector<nsApp::nsCombat::HitPartShape> vecShapeList;
		for (int i = 0; i < iNumShape; i++)
		{
			if (!stFile.EnterArrayElement(sPartsArrayName_, i))
				continue;

			nsApp::nsCombat::HitPartShape stShape;
			stShape.enPart_ = FindPartByName(stFile.GetString("part", "Body"));
			stShape.fBottom_ = stFile.GetFloat("bottom", 0.0f);
			stShape.fTop_ = stFile.GetFloat("top", 0.0f);
			stShape.fRadius_ = stFile.GetFloat("radius", 0.0f);

			stFile.Leave();

			/* 名前が間違っている、または形になっていない行は捨てる。*/
			if (stShape.enPart_ == nsApp::nsCombat::EnHitPart::None || stShape.fTop_ <= stShape.fBottom_ || stShape.fRadius_ <= 0.0f)
				continue;

			vecShapeList.push_back(stShape);
		}

		/* 1つも読めなければ既定の形を残す。*/
		if (vecShapeList.empty())
			return;

		/* Clear() で倍率も初期化されるので、控えてから形を入れ替えて戻す。*/
		float aRateList[static_cast<int>(nsApp::nsCombat::EnHitPart::Num)] = {};
		for (int i = 0; i < static_cast<int>(nsApp::nsCombat::EnHitPart::Num); i++)
			aRateList[i] = stSet.GetDamageRate(static_cast<nsApp::nsCombat::EnHitPart>(i));

		stSet.Clear();

		/* JSONから読めた形を積み直す。*/
		for (const nsApp::nsCombat::HitPartShape& stShape : vecShapeList)
			stSet.AddPartShape(stShape);

		/* 控えておいた倍率を書き戻す。*/
		for (int i = 0; i < static_cast<int>(nsApp::nsCombat::EnHitPart::Num); i++)
			stSet.SetDamageRate(static_cast<nsApp::nsCombat::EnHitPart>(i), aRateList[i]);
	}


	/**
	 * @brief レイと縦円柱の当たり判定。
	 *        円柱は水平方向が円、上下が平らな蓋になっている閉じた形として扱う。
	 * @param vRayStart     判定の起点。
	 * @param vRayDirection 進む向き(正規化済み)。
	 * @param fMaxDistance  調べる最大距離。
	 * @param vCenterXZ     円柱の中心(水平位置。yは使わない)。
	 * @param fBottomY      円柱の下端の高さ(ワールド)。
	 * @param fTopY         円柱の上端の高さ(ワールド)。
	 * @param fRadius       円柱の半径。
	 * @param fOutDistance  起点から命中点までの距離を受け取る。
	 * @return 当たっていたらtrue。
	 */
	bool IntersectRayAndCylinder(const Vector3& vRayStart, const Vector3& vRayDirection, float fMaxDistance,
		const Vector3& vCenterXZ, float fBottomY, float fTopY, float fRadius, float& fOutDistance)
	{
		/* 調べる区間。ここを水平の円と上下の蓋で削っていき、残れば命中。*/
		float fEnter = 0.0f;
		float fExit = fMaxDistance;

		/* 水平方向: 起点から円の中心へのずれと、進む向きの水平成分で2次方程式を解く。*/
		const float fOffsetX = vRayStart.x - vCenterXZ.x;
		const float fOffsetZ = vRayStart.z - vCenterXZ.z;
		const float fA = vRayDirection.x * vRayDirection.x + vRayDirection.z * vRayDirection.z;
		const float fB = 2.0f * (fOffsetX * vRayDirection.x + fOffsetZ * vRayDirection.z);
		const float fC = fOffsetX * fOffsetX + fOffsetZ * fOffsetZ - fRadius * fRadius;

		/* 真上・真下を向いている場合は水平に動かないので、円の内側にいるかだけを見る。*/
		if (fA <= 0.000001f)
		{
			if (fC > 0.0f)
				return false;
		}
		else
		{
			const float fDiscriminant = fB * fB - 4.0f * fA * fC;
			if (fDiscriminant < 0.0f)
				return false;

			const float fRoot = sqrtf(fDiscriminant);
			const float fNear = (-fB - fRoot) / (2.0f * fA);
			const float fFar = (-fB + fRoot) / (2.0f * fA);

			if (fNear > fEnter)
				fEnter = fNear;

			if (fFar < fExit)
				fExit = fFar;

			if (fEnter > fExit)
				return false;
		}

		/* 上下方向: 下端と上端の蓋で区間を削る。*/
		if (fabsf(vRayDirection.y) <= 0.000001f)
		{
			/* 上下に動かないので、高さが範囲内にあるかだけを見る。*/
			if (vRayStart.y < fBottomY || vRayStart.y > fTopY)
				return false;
		}
		else
		{
			float fToBottom = (fBottomY - vRayStart.y) / vRayDirection.y;
			float fToTop = (fTopY - vRayStart.y) / vRayDirection.y;

			/* 下を向いていると前後が入れ替わるので並べ直す。*/
			if (fToBottom > fToTop)
			{
				const float fSwap = fToBottom;
				fToBottom = fToTop;
				fToTop = fSwap;
			}

			if (fToBottom > fEnter)
				fEnter = fToBottom;

			if (fToTop < fExit)
				fExit = fToTop;

			if (fEnter > fExit)
				return false;
		}

		/* 区間が後ろ側だけ、または射程の外なら当たっていない。*/
		if (fExit < 0.0f || fEnter > fMaxDistance)
			return false;

		fOutDistance = (fEnter > 0.0f) ? fEnter : 0.0f;

		return true;
	}
}


namespace nsApp
{
	namespace nsCombat
	{


		void HitBoxSet::AddPartShape(const HitPartShape& stShape)
		{
			/* 部位の形を末尾へ足す。並び順は判定に影響しない。*/
			vecPartShapeList_.push_back(stShape);
		}


		void HitBoxSet::SetDamageRate(EnHitPart enPart, float fRate)
		{
			/* 部位でなければ無視する。*/
			if (enPart >= EnHitPart::Num)
				return;

			/* 部位を添字にして倍率を覚える。*/
			aDamageRateList_[static_cast<int>(enPart)] = fRate;
		}


		void HitBoxSet::Clear()
		{
			/* 部位の形を捨てる。*/
			vecPartShapeList_.clear();

			/* 倍率は等倍へ戻す。形が無い状態で参照されても破綻しないようにするため。*/
			const int iNumPart = static_cast<int>(EnHitPart::Num);
			for (int i = 0; i < iNumPart; i++)
				aDamageRateList_[i] = 1.0f;
		}


		float HitBoxSet::GetDamageRate(EnHitPart enPart) const
		{
			/* 部位でなければ等倍。*/
			if (enPart >= EnHitPart::Num)
				return 1.0f;

			/* 覚えておいた倍率を返す。*/
			return aDamageRateList_[static_cast<int>(enPart)];
		}


		float HitBoxSet::GetHeight() const
		{
			float fHeight = 0.0f;
			for (const HitPartShape& stShape : vecPartShapeList_)
			{
				if (stShape.fTop_ > fHeight)
					fHeight = stShape.fTop_;
			}

			return fHeight;
		}


		bool HitBoxSet::FindHitPart(const Vector3& vRayStart, const Vector3& vRayDirection, float fMaxDistance, const Vector3& vFootPosition, HitResult& stOutResult) const
		{
			stOutResult = HitResult();

			/* 部位が1つも無ければ当たらない。*/
			if (vecPartShapeList_.empty())
				return false;

			/* 一番手前で当たった部位を探す。*/
			float fNearest = fMaxDistance;
			bool bIsHit = false;
			EnHitPart enHitPart = EnHitPart::None;

			for (const HitPartShape& stShape : vecPartShapeList_)
			{
				/* 部位の高さは足元からの値なので、足元の座標を足してワールドの高さにする。*/
				const float fBottomY = vFootPosition.y + stShape.fBottom_;
				const float fTopY = vFootPosition.y + stShape.fTop_;

				float fDistance = 0.0f;
				if (!IntersectRayAndCylinder(vRayStart, vRayDirection, fNearest, vFootPosition, fBottomY, fTopY, stShape.fRadius_, fDistance))
					continue;

				/* より手前で当たったものを採用する。*/
				if (fDistance > fNearest)
					continue;

				fNearest = fDistance;
				enHitPart = stShape.enPart_;
				bIsHit = true;
			}

			if (!bIsHit)
				return false;

			stOutResult.bIsHit_ = true;
			stOutResult.enPart_ = enHitPart;
			stOutResult.fDistance_ = fNearest;
			stOutResult.vHitPoint_ = vRayStart + vRayDirection * fNearest;
			stOutResult.fDamageRate_ = GetDamageRate(enHitPart);

			return true;
		}


		void HitBoxSet::Load()
		{
			/* まず既定の形で埋める。JSONが無くても、この形でそのまま遊べる。*/
			BuildHumanDefault(stInfectedHitBoxSet_);
			BuildHumanDefault(stSurvivorHitBoxSet_);

			/* 以降は読み込み済みとして扱う(JSONが無くてもここまでで遊べる)。*/
			bIsLoaded_ = true;

			/* JSONを開く。無ければ既定値のままにする。*/
			nsData::ParameterFile stFile;
			if (!stFile.Load(sHitBoxFilePath_))
			{
				DebugPrintW(L"[HitBoxSet] hitbox.json が読めないので既定値を使います。\n");
				return;
			}

			/* 当たり判定一覧のノードへ潜る。*/
			if (!stFile.Enter(sHitBoxSetsNodeName_))
			{
				DebugPrintW(L"[HitBoxSet] hitbox.json に当たり判定一覧が無いので既定値を使います。\n");
				return;
			}

			if (stFile.Enter(sInfectedSetName_))
			{
				OverwriteFromFile(stFile, stInfectedHitBoxSet_);
				stFile.Leave();
			}

			if (stFile.Enter(sSurvivorSetName_))
			{
				OverwriteFromFile(stFile, stSurvivorHitBoxSet_);
				stFile.Leave();
			}
		}


		const HitBoxSet& HitBoxSet::GetShared(CharacterModelType enModelType)
		{
			/* 初回の呼び出しでJSONを読み込む。*/
			if (!bIsLoaded_)
				Load();

			return (enModelType == CharacterModelType::Survivor) ? stSurvivorHitBoxSet_ : stInfectedHitBoxSet_;
		}


		const char* HitBoxSet::GetPartName(EnHitPart enPart)
		{
			/* 部位でなければ空文字を返す。*/
			if (enPart >= EnHitPart::Num)
				return "";

			return sPartNameList_[static_cast<int>(enPart)];
		}
	}
}
