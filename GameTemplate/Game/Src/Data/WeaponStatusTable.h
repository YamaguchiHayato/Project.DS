#pragma once
#include "Weapon.h"

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file   WeaponStatusTable.h
		 * @brief  武器ステータス表。Assets/data/weapon.json から武器のパラメータを読み込んで保持する。
		 *         Weapon はこの表からパラメータを引くだけなので、
		 *         数値の調整はJSONを書き換えるだけで済み、リビルドが要らない。
		 *         JSONが無い・項目が足りない場合は、このクラスが持つ既定値がそのまま使われる。
		 * @author Izumida Kiryu
		 * @date   2026/09/02
		 */
		class WeaponStatusTable
		{
		public:
			/* コンストラクタとデストラクタ。*/
			WeaponStatusTable() = default;
			virtual ~WeaponStatusTable() = default;


		public:
			/**
			 * @brief 武器のパラメータを取得する。初回の呼び出しでJSONを読み込む。
			 * @param enType 武器の種類。
			 * @return その武器のパラメータ。
			 */
			static const nsWeapon::WeaponStatus& Get(nsWeapon::EnWeaponType enType);

			/**
			 * @brief JSONを読み直す(既定値へ戻してから上書きし直す)。
			 *        ゲーム中に呼ぶと、すでに配ったWeaponStatusが指す武器名・リコイルパターンの実体が
			 *        作り直されて無効になる。呼ぶのは起動時だけにすること。
			 */
			static void Load();

			/**
			 * @brief 武器の種類に対応するJSONのキー名を取得する。
			 * @param enType 武器の種類。
			 * @return キー名。
			 */
			static const char* GetTypeName(nsWeapon::EnWeaponType enType);
		};
	}
}
