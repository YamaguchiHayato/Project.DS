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
			 *        { 武器名, 発射間隔(秒), 最大弾数, リロード時間(秒), 威力, フルオートか, モデルパス, サイズ倍率, 前方距離 }
			 *        サイズ倍率は自動サイズ合わせに対する倍率(1.0=標準。大きく見せたい銃は1.2等)。
			 */
			const WeaponStatus WEAPON_STATUS_TABLE[] =
			{
				/* Handgun。      */ { "Handgun",      0.25f,  8, 2.2f, 35, false, "Assets/modelData/gun/subWeapon/m1911.tkm",   1.0f,  55.0f },
				/* AssaultRifle。 */ { "AssaultRifle", 0.10f, 30, 2.8f, 20, true,  "Assets/modelData/gun/mainWeapon/M4A1.tkm",    2.0f,  35.0f },
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
			fFireTimer_ = 0.0f;
			fReloadTimer_ = 0.0f;
			bIsReloading_ = false;
		}


		void Weapon::Update(float fDeltaTime)
		{
			/* 発射クールタイムを進める。*/
			if (fFireTimer_ > 0.0f)
				fFireTimer_ -= fDeltaTime;

			/* リロード中ならリロードタイマーを進める。*/
			if (bIsReloading_)
			{
				fReloadTimer_ -= fDeltaTime;

				/* リロード完了で弾を満タンにする。*/
				if (fReloadTimer_ <= 0.0f)
				{
					bIsReloading_ = false;
					iCurrentAmmo_ = stStatus_.iMaxAmmo_;
				}
			}
		}


		bool Weapon::Fire(const Vector3& vPosition, const Vector3& vDirection)
		{
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


		void Weapon::Reload()
		{
			/* すでにリロード中なら何もしない。*/
			if (bIsReloading_)
				return;

			/* 弾が満タンならリロード不要。*/
			if (iCurrentAmmo_ >= stStatus_.iMaxAmmo_)
				return;

			/* リロードを開始する。*/
			bIsReloading_ = true;
			fReloadTimer_ = stStatus_.fReloadTime_;
		}
	}
}
