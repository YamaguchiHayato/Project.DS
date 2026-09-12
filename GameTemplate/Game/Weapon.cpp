#include "stdafx.h"
#include "Weapon.h"
#include "Src/Data/WeaponStatusTable.h"

namespace nsApp
{
	namespace nsWeapon
	{
		void Weapon::Init(EnWeaponType enType)
		{
			/* 種類を保持し、ステータス表(JSON)からパラメータを引く。*/
			enType_ = enType;
			stStatus_ = nsData::WeaponStatusTable::Get(enType);

			/* 弾を満タンにし、タイマー類を初期化する。*/
			iCurrentAmmo_ = stStatus_.iMaxAmmo_;
			iReserveAmmo_ = stStatus_.iMaxReserveAmmo_;
			fFireTimer_ = 0.0f;
			fReloadTimer_ = 0.0f;
			iRecoilIndex_ = 0;
			iBurstRemain_ = 0;
			fRecoilResetTimer_ = 0.0f;
			bIsReloading_ = false;
		}


		void Weapon::Update(float fDeltaTime, float fActionSpeedRate)
		{
			/* 手の動き(構え・リロード)だけ倍率を掛ける。銃の連射速度は薬だけでは変わらない。*/
			const float fActionDelta = fDeltaTime * fActionSpeedRate;

			/* 構え終わるまでの時間を進める。*/
			if (fDeployTimer_ > 0.0f)
				fDeployTimer_ -= fActionDelta;

			/* 発射クールタイムを進める。*/
			if (fFireTimer_ > 0.0f)
				fFireTimer_ -= fDeltaTime;

			/* 撃つのをやめてしばらく経ったら、リコイルパターンを最初へ戻す。*/
			fRecoilResetTimer_ += fDeltaTime;
			if (fRecoilResetTimer_ >= stStatus_.fRecoilResetTime_)
				iRecoilIndex_ = 0;

			/* リロード中ならリロードタイマーを進め、時間が来たら装填する。*/
			if (bIsReloading_)
			{
				fReloadTimer_ -= fActionDelta;
				if (fReloadTimer_ <= 0.0f)
					FinishReloadStep();
			}
		}


		void Weapon::FinishReloadStep()
		{
			/* マガジンを満たすのに必要な数。*/
			const int iNeed = stStatus_.iMaxAmmo_ - iCurrentAmmo_;

			/* 1回で入れる数。1発ずつ装填する銃は1、普通の銃は必要なぶん全部。*/
			int iLoad = stStatus_.bIsShellReload_ ? 1 : iNeed;

			/* メイン武器は予備弾から補充し、足りなければあるぶんだけ入れる。サブ武器は無限。*/
			if (!IsInfiniteReserve())
			{
				if (iLoad > iReserveAmmo_)
					iLoad = iReserveAmmo_;

				iReserveAmmo_ -= iLoad;
			}

			iCurrentAmmo_ += iLoad;

			/* 1発ずつ装填する銃は、まだ入る余地と予備があればもう1発ぶん続ける。*/
			const bool bCanContinue = stStatus_.bIsShellReload_
				&& (iCurrentAmmo_ < stStatus_.iMaxAmmo_)
				&& (IsInfiniteReserve() || iReserveAmmo_ > 0);

			if (bCanContinue)
			{
				fReloadTimer_ = stStatus_.fReloadTime_;
				return;
			}

			bIsReloading_ = false;
			fReloadTimer_ = 0.0f;
		}


		bool Weapon::Fire(const Vector3& vPosition, const Vector3& vDirection)
		{
			/* 構えている途中は撃てない。*/
			if (IsDeploying())
				return false;

			/* リロード中は撃てない。ただし1発ずつ装填する銃は、弾が入っていれば装填をやめて撃てる(本家のショットガンと同じ)。*/
			if (bIsReloading_)
			{
				if (!stStatus_.bIsShellReload_ || iCurrentAmmo_ <= 0)
					return false;

				CancelReload();
			}

			/* クールタイム中は撃てない。*/
			if (fFireTimer_ > 0.0f)
				return false;

			/* 弾切れなら自動でリロードを開始し、今回は撃てない扱いにする。点射の途中でもそこで終わる。*/
			if (iCurrentAmmo_ <= 0)
			{
				iBurstRemain_ = 0;
				Reload();
				return false;
			}

			/* 弾を1発消費する。*/
			iCurrentAmmo_--;

			/*
			 * 次に撃てるまでの時間。
			 * 点射は「引き金を引いた瞬間に残りの発数を予約し、短い間隔で撃ち切ってから通常の間隔を空ける」。
			 */
			if (stStatus_.enFireMode_ == EnFireMode::Burst)
			{
				if (iBurstRemain_ <= 0)
					iBurstRemain_ = stStatus_.iBurstCount_ - 1;
				else
					iBurstRemain_--;

				fFireTimer_ = (iBurstRemain_ > 0) ? stStatus_.fBurstInterval_ : stStatus_.fFireInterval_;
			}
			else
			{
				fFireTimer_ = stStatus_.fFireInterval_;
			}

			/* この1発ぶんの跳ね方をパターンから取り出し、次の段へ進める。*/
			if (stStatus_.pRecoilPattern_ != nullptr && stStatus_.iRecoilPatternCount_ > 0)
			{
				/* 最後まで撃ち切ったら、最終段を繰り返す。*/
				if (iRecoilIndex_ >= stStatus_.iRecoilPatternCount_)
					iRecoilIndex_ = stStatus_.iRecoilPatternCount_ - 1;

				stLastRecoilStep_ = stStatus_.pRecoilPattern_[iRecoilIndex_];
				iRecoilIndex_++;
			}

			/* 撃ったので、パターンが戻るまでの時間を数え直す。*/
			fRecoilResetTimer_ = 0.0f;

			/*
			 * 命中判定はここでは行わない。散弾数・貫通数を見て弾を飛ばすのは持ち主(Player)の役目。
			 * DebugPrintW は第1引数の「書式文字列」を内部で整形して直接出力する。
			 * 書式はワイド文字列(L"...")で書き、ナロー文字列(const char* の pName_)は
			 * ワイド書式では %hs を使う(%s だと wchar_t* 扱いになり文字化けする)。
			 */
			DebugPrintW(L"[Weapon] %hs Fire! ammo=%d pos(%.1f,%.1f,%.1f) dir(%.2f,%.2f,%.2f)\n",
				stStatus_.pName_, iCurrentAmmo_,
				vPosition.x, vPosition.y, vPosition.z,
				vDirection.x, vDirection.y, vDirection.z);

			return true;
		}


		float Weapon::CalcFalloffRate(float fDistance) const
		{
			/* 落ち始めの距離が無い、または幅が無ければ落ちない。*/
			if (stStatus_.fFalloffStart_ <= 0.0f || stStatus_.fFalloffEnd_ <= stStatus_.fFalloffStart_)
				return 1.0f;

			if (fDistance <= stStatus_.fFalloffStart_)
				return 1.0f;

			if (fDistance >= stStatus_.fFalloffEnd_)
				return stStatus_.fFalloffMinRate_;

			/* 落ち始めから下限まで直線で落とす。*/
			const float fRate = (fDistance - stStatus_.fFalloffStart_) / (stStatus_.fFalloffEnd_ - stStatus_.fFalloffStart_);
			return 1.0f - (1.0f - stStatus_.fFalloffMinRate_) * fRate;
		}


		void Weapon::CancelReload()
		{
			bIsReloading_ = false;
			fReloadTimer_ = 0.0f;
		}


		void Weapon::Deploy()
		{
			/* 持ち替えたのでリロードは中断する。点射の予約も消える。*/
			CancelReload();
			iBurstRemain_ = 0;

			/* 構え終わるまで撃てない時間を設定する。*/
			fDeployTimer_ = stStatus_.fDeployTime_;

			/* 持ち替えたらリコイルパターンも最初へ戻す。*/
			iRecoilIndex_ = 0;
		}


		void Weapon::Reload()
		{
			/* 構えている途中はリロードできない。*/
			if (IsDeploying())
				return;

			/* すでにリロード中なら何もしない。*/
			if (bIsReloading_)
				return;

			/* 弾が満タンならリロード不要。*/
			if (iCurrentAmmo_ >= stStatus_.iMaxAmmo_)
				return;

			/* 予備弾が尽きていればリロードできない(サブ武器は無限なので常に可能)。*/
			if (!IsInfiniteReserve() && iReserveAmmo_ <= 0)
				return;

			/* リロードを開始する。点射の予約は消える。*/
			bIsReloading_ = true;
			iBurstRemain_ = 0;
			fReloadTimer_ = stStatus_.fReloadTime_;
		}
	}
}
