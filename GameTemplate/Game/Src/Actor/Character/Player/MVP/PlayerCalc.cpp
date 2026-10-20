#include "stdafx.h"
#include "PlayerCalc.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Combat/HitBoxSet.h"
#include <algorithm>
#include "Tracer.h"
#include "Weapon.h"

namespace
{
	const float PI = 3.14159265f; //! 円周率。
	/* リロード動作の区切り。0=リロード開始、1=完了間際。*/
	const float RELOAD_PULL_END = 0.28f;			//! ここまでで銃を下げ切る(マガジンを抜く)。
	const float RELOAD_INSERT_START = 0.34f;		//! ここからマガジンを挿し込む。
	const float RELOAD_INSERT_END = 0.52f;			//! ここで挿し込み終わる。
	const float RELOAD_RAISE_START = 0.58f;			//! ここから構え直す。
	const float RELOAD_RAISE_END = 0.96f;			//! ここで構え終わる。
	const float RELOAD_SETTLE_START = 0.90f;		//! ここから戻りの行き過ぎが出る。
	const float ADS_RECOIL_RATE = 0.6f; //! 覗き込み中の反動の倍率(構えるほど跳ねが小さくなる)。

	/**
	 * @brief 両端がなめらかな 0〜1 の補間値を作る。
	 * @param fStart 動きが始まる位置。
	 * @param fEnd   動きが終わる位置。
	 * @param fValue いまの位置。
	 * @return 0〜1の値。範囲の外では端の値。
	 */
	float SmoothStep(float fStart, float fEnd, float fValue)
	{
		if (fEnd <= fStart)
			return (fValue < fStart) ? 0.0f : 1.0f;

		const float fRate = (fValue - fStart) / (fEnd - fStart);
		if (fRate <= 0.0f)
			return 0.0f;

		if (fRate >= 1.0f)
			return 1.0f;

		/* 3次式で両端をなめらかにする。*/
		return fRate * fRate * (3.0f - 2.0f * fRate);
	}

	/**
	 * @brief 0→1→0 と山なりに変化する値を作る(短い動きの1往復に使う)。
	 * @param fStart 動きが始まる位置。
	 * @param fEnd   動きが終わる位置。
	 * @param fValue いまの位置。
	 * @return 0〜1の値。範囲の外では0。
	 */
	float PulseCurve(float fStart, float fEnd, float fValue)
	{
		if (fValue <= fStart || fValue >= fEnd || fEnd <= fStart)
			return 0.0f;

		return sinf(PI * (fValue - fStart) / (fEnd - fStart));
	}
}


namespace nsApp
{
	namespace nsActor
	{
		float PlayerCalc::GetCurrentSpread() const
		{
			/* nullの場合、拡散無し。*/
			if (pPlayer_ == nullptr)
				return 0.0f;

			/* 武器が無ければ拡散も無い。*/
			if (pPlayer_->GetWeaponInventory().GetCurrentWeapon() == nullptr)
				return 0.0f;

			/* 覗き込みの度合いで腰だめとADSの拡散を混ぜ、姿勢と移動の倍率を掛け、連射ぶんを足す。*/
			return (pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetSpreadHip()
				+ (pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetSpreadAds() - pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetSpreadHip()) * pPlayer_->GetAdsRate())
				* pPlayer_->GetStanceSpreadRate()
				+ pPlayer_->GetSpreadShot();
		}


		Vector3 PlayerCalc::MakeSpreadDirection(const Vector3& vAimDir) const
		{
			/* nullの場合、拡散させない。*/
			if (pPlayer_ == nullptr)
				return vAimDir;

			/* 武器が無ければ拡散させない。*/
			if (pPlayer_->GetWeaponInventory().GetCurrentWeapon() == nullptr)
				return vAimDir;

			/* いまの拡散角を取り出す。*/
			if (GetCurrentSpread() <= 0.0001f)
				return vAimDir;

			/* 照準を軸に、円錐の内側へランダムにずらす。*/
			workAngle_ = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f * PI;
			workRadius_ = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * GetCurrentSpread();

			/* 照準に垂直な2軸を作る。*/
			workRight_ = { cosf(pPlayer_->GetCameraYaw()), 0.0f, -sinf(pPlayer_->GetCameraYaw()) };
			workUp_ = vAimDir;
			workUp_.Cross(workRight_);

			/* 2軸へずらして方向を作り直す。*/
			workResult_ = vAimDir;
			workResult_ += workRight_ * (cosf(workAngle_) * workRadius_);
			workResult_ += workUp_ * (sinf(workAngle_) * workRadius_);
			workResult_.Normalize();

			return workResult_;
		}


		void PlayerCalc::CalcReloadMotion(float fReloadRate, float& fOutLower, float& fOutInsert, float& fOutSettle) const
		{
			/*
			 * 銃を下げている度合い。
			 * 前半で下げ切って(マガジンを抜く)、後半で構え直すぶんを引く。
			 */
			fOutLower = SmoothStep(0.0f, RELOAD_PULL_END, fReloadRate) - SmoothStep(RELOAD_RAISE_START, RELOAD_RAISE_END, fReloadRate);

			/* マガジンを挿し込む短い突き上げ。0→1→0 と1往復する。*/
			fOutInsert = PulseCurve(RELOAD_INSERT_START, RELOAD_INSERT_END, fReloadRate);

			/* 構え直した勢いで少し行き過ぎてから収まる。*/
			fOutSettle = PulseCurve(RELOAD_SETTLE_START, 1.0f, fReloadRate);
		}


		void PlayerCalc::ApplyFireRecoil(nsWeapon::Weapon* pWeapon)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 武器が無ければ反動も無い。*/
			if (pWeapon == nullptr)
				return;

			 /* 覗き込み中は反動を抑える。*/
			workAdsMul_ = 1.0f - (1.0f - ADS_RECOIL_RATE) * pPlayer_->GetAdsRate();

			/* パターンの倍率に武器の反動量を掛けて、視点を跳ね上げる。*/
			pPlayer_->AddRecoilPitch(pWeapon->GetRecoilPitch() * pWeapon->GetLastRecoilStep().fPitch_ * workAdsMul_);
			pPlayer_->AddRecoilYaw(pWeapon->GetRecoilYaw() * pWeapon->GetLastRecoilStep().fYaw_ * workAdsMul_);

			/* 銃を手前へ下げる(次のフレームから元へ戻っていく)。*/
			pPlayer_->SetWeaponKickBack(pWeapon->GetKickBack());
		}


		void PlayerCalc::FindHitEnemies(const Vector3& vRayStart, const Vector3& vRayDirection, int iMaxCount, std::vector<HitEnemy>& vecOutHits)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			vecOutHits.clear();

			/* 敵の部位別当たり判定(形とダメージ倍率)を取り出す。*/
			/* 射程内で弾道に触れる敵を全部集める。*/
			for (CommonEnemy* pEnemy : FindGOs<CommonEnemy>("commonEnemy"))
			{
				if (pEnemy == nullptr)
					continue;

				/* 敵1体ぶんの部位別判定。足元の座標を基準に頭・胴・脚を並べて判定する。*/
				workHit_ = HitEnemy();
				if (!nsCombat::HitBoxSet::GetShared(CharacterModelType::Common).FindHitPart(vRayStart, vRayDirection, pPlayer_->GetPlayerStatus().fWeaponRange_, pEnemy->GetPosition(), workHit_.stResult_))
					continue;

				/* 命中した敵を追加する。*/
				workHit_.pEnemy_ = pEnemy;
				vecOutHits.push_back(workHit_);
			}

			/* 手前から順に並べ、貫通できる数だけ残す。*/
			std::sort(vecOutHits.begin(), vecOutHits.end(),[](const HitEnemy& stA, const HitEnemy& stB){
					return stA.stResult_.fDistance_ < stB.stResult_.fDistance_;
				});

			/* 貫通できる数は最低1体は残す。*/
			if (iMaxCount < 1)
				iMaxCount = 1;

			/* 貫通できる数だけ残す。*/
			if (static_cast<int>(vecOutHits.size()) > iMaxCount)
				vecOutHits.resize(iMaxCount);
		}


		void PlayerCalc::FireHitScan(nsWeapon::Weapon* pWeapon, const Vector3& vEyePos, const Vector3& vMuzzlePos, const Vector3& vAimDir)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr || pWeapon == nullptr)
				return;

			/* 拡散のぶんだけ照準をばらつかせた、実際の弾道。*/
			workShotDir_ = MakeSpreadDirection(vAimDir);

			/* レイの終点(最大射程)。命中したら一番奥の命中点に置き換える。*/
			workHitPoint_ = vEyePos + workShotDir_ * pPlayer_->GetPlayerStatus().fWeaponRange_;

			/* 弾道に触れる敵を手前から拾う。貫通しない武器は1体、貫通する武器はその数だけ奥まで。*/
			FindHitEnemies(vEyePos, workShotDir_, pWeapon->GetPenetrateCount() + 1, workHits_);

			/* 命中した敵の数だけループしてダメージを与える。*/
			for (workHitIndex_ = 0; workHitIndex_ < static_cast<int>(workHits_.size()); workHitIndex_++)
			{
				/* 命中点を更新する。*/
				workHitPoint_ = workHits_[workHitIndex_].stResult_.vHitPoint_;

				/* 当たった部位の倍率(頭なら大ダメージ、脚なら効きが悪い)と、距離による減衰を掛ける。*/
				workDamage_ = static_cast<int>(static_cast<float>(pWeapon->GetAttackPower())*(workHits_[workHitIndex_].stResult_.fDamageRate_* pWeapon->CalcFalloffRate(workHits_[workHitIndex_].stResult_.fDistance_)));

				/* 頭に当たったかはUIと演出で使う。*/
				pPlayer_->DealBulletDamage(workHits_[workHitIndex_].pEnemy_,workDamage_,workHitPoint_,workShotDir_,workHits_[workHitIndex_].stResult_.enPart_ == nsCombat::EnHitPart::Head);
			}

			/* 見せるためのトレーサー(曳光弾)を一瞬だけ表示する。貫通したときは一番奥の敵まで伸びる。*/
			NewGO<nsWeapon::Tracer>(0, "tracer")->Setup(vMuzzlePos, workHitPoint_);
		}	
	}
}