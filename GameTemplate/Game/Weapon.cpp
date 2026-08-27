#include "stdafx.h"
#include "Weapon.h"

namespace nsApp
{
	namespace nsWeapon
	{
		namespace
		{
			/**
			 * @brief 武器のステータステーブル。EnWeaponTypeの並び順と一致させること。
			 *        銃を増やすときはEnWeaponTypeに種類を足し、ここに1行追加する。
			 *        { 武器名, 発射間隔(秒), 区分, 最大弾数, 予備弾数, リロード時間(秒), 構え時間(秒), 威力, フルオートか, モデルパス, サイズ倍率, 前方距離, 反動(上), 反動(左右), キックバック, 拡散(腰だめ), 拡散(ADS), 拡散増加/発, ADS画角率, ADS速度率, リコイルパターン, 段数 }
			 *        サイズ倍率は自動サイズ合わせに対する倍率(1.0=標準。大きく見せたい銃は1.2等)。
			 */
			const float kRecoilResetTime = 0.35f;	//! 撃つのをやめてからパターンが最初へ戻るまでの時間(秒)。

			/*
			 * ハンドガンのリコイルパターン。
			 * 単発なので素直に真上へ跳ね、撃ち続けると少しずつ左右へ散る。
			 */
			const RecoilStep HANDGUN_RECOIL_PATTERN[] =
			{
				{ 1.00f,  0.00f },
				{ 1.00f,  0.25f },
				{ 0.95f, -0.30f },
				{ 0.90f,  0.45f },
				{ 0.85f, -0.50f },
				{ 0.80f,  0.35f },
			};

			/*
			 * アサルトライフルのリコイルパターン。
			 * 最初の数発は真上へ強く跳ね、そのあと右へ流れてから左へ返す。
			 * この形を覚えて逆へ動かせば制御できる、という遊びを狙っている。
			 */
			const RecoilStep ASSAULT_RIFLE_RECOIL_PATTERN[] =
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

			const WeaponStatus WEAPON_STATUS_TABLE[] =
			{
				/* Handgun。      */ { "Handgun",      0.25f, EnWeaponSlot::Sub,   8,  0, 2.2f, 0.35f, 18, false, "Assets/modelData/gun/subWeapon/m1911.tkm",   1.0f,  55.0f, 0.030f, 0.010f, 10.0f, 0.035f, 0.004f, 0.006f, 0.80f, 0.70f, HANDGUN_RECOIL_PATTERN, _countof(HANDGUN_RECOIL_PATTERN) },
				/* AssaultRifle。 */ { "AssaultRifle", 0.10f, EnWeaponSlot::Main, 30, 180, 2.8f, 0.55f, 20, true,  "Assets/modelData/gun/mainWeapon/M4A1.tkm",    2.0f,  35.0f, 0.014f, 0.008f,  6.0f, 0.050f, 0.008f, 0.004f, 0.70f, 0.60f, ASSAULT_RIFLE_RECOIL_PATTERN, _countof(ASSAULT_RIFLE_RECOIL_PATTERN) },
			};

			/* テーブルの要素数がEnWeaponType::Numと一致しているかをコンパイル時に検査する。*/
			static_assert(
				sizeof(WEAPON_STATUS_TABLE) / sizeof(WEAPON_STATUS_TABLE[0]) == static_cast<size_t>(EnWeaponType::Num),
				"WEAPON_STATUS_TABLE の行数と EnWeaponType::Num が一致していません。");
		}


		void Weapon::Init(EnWeaponType enType)
		{
			/* 種類を保持し、テーブルからパラメータを引く。*/
			enType_ = enType;
			stStatus_ = WEAPON_STATUS_TABLE[static_cast<size_t>(enType)];

			/* 弾を満タンにし、タイマー類を初期化する。*/
			iCurrentAmmo_ = stStatus_.iMaxAmmo_;
			iReserveAmmo_ = stStatus_.iMaxReserveAmmo_;
			fFireTimer_ = 0.0f;
			fReloadTimer_ = 0.0f;
			iRecoilIndex_ = 0;
			fRecoilResetTimer_ = 0.0f;
			bIsReloading_ = false;
		}


		void Weapon::Update(float fDeltaTime)
		{
			/* 構え終わるまでの時間を進める。*/
			if (fDeployTimer_ > 0.0f)
				fDeployTimer_ -= fDeltaTime;

			/* 発射クールタイムを進める。*/
			if (fFireTimer_ > 0.0f)
				fFireTimer_ -= fDeltaTime;

			/* 撃つのをやめてしばらく経ったら、リコイルパターンを最初へ戻す。*/
			fRecoilResetTimer_ += fDeltaTime;
			if (fRecoilResetTimer_ >= kRecoilResetTime)
				iRecoilIndex_ = 0;

			/* リロード中ならリロードタイマーを進める。*/
			if (bIsReloading_)
			{
				fReloadTimer_ -= fDeltaTime;

				/* リロード完了で弾を満タンにする。*/
				if (fReloadTimer_ <= 0.0f)
				{
					bIsReloading_ = false;

					/* マガジンを満たすのに必要な数を求める。*/
					const int iNeed = stStatus_.iMaxAmmo_ - iCurrentAmmo_;

					/* サブ武器は予備弾が無限なので、常に満タンにする。*/
					if (IsInfiniteReserve())
					{
						iCurrentAmmo_ = stStatus_.iMaxAmmo_;
					}
					else
					{
						/* メイン武器は予備弾から補充し、足りなければあるぶんだけ入れる。*/
						const int iLoad = (iNeed < iReserveAmmo_) ? iNeed : iReserveAmmo_;
						iCurrentAmmo_ += iLoad;
						iReserveAmmo_ -= iLoad;
					}
				}
			}
		}


		bool Weapon::Fire(const Vector3& vPosition, const Vector3& vDirection)
		{
			/* 構えている途中は撃てない。*/
			if (IsDeploying())
				return false;

			/* リロード中は撃てない。*/
			if (bIsReloading_)
				return false;

			/* クールタイム中は撃てない。*/
			if (fFireTimer_ > 0.0f)
				return false;

			/* 弾切れなら自動でリロードを開始し、今回は撃てない扱いにする。*/
			if (iCurrentAmmo_ <= 0)
			{
				Reload();
				return false;
			}

			/* 弾を1発消費し、クールタイムを設定する。*/
			iCurrentAmmo_--;
			fFireTimer_ = stStatus_.fFireInterval_;

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
			 * TODO: ここで実際の弾Actor生成 or レイキャストによる命中判定を行う。
			 *       当たり判定の共有仕様(敵側との接続)が決まったら、
			 *       vPosition/vDirection と GetAttackPower() を使ってダメージを与える。
			 */
			/*
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


		void Weapon::Deploy()
		{
			/* 持ち替えたのでリロードは中断する。*/
			bIsReloading_ = false;
			fReloadTimer_ = 0.0f;

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

			/* リロードを開始する。*/
			bIsReloading_ = true;
			fReloadTimer_ = stStatus_.fReloadTime_;
		}
	}
}
