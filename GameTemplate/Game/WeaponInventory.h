#pragma once
#include <vector>
#include "Weapon.h"

namespace nsApp
{
	namespace nsWeapon
	{
		/**
		 * @file   WeaponInventory.h
		 * @brief  所持している武器のリストを管理し、現在の武器の切り替え・発射を
		 *         まとめて扱うクラス。データ駆動のWeaponを値で複数所持する。
		 *         Player/Enemyどちらからも使える。
		 * @author Izumida Kiryu
		 * @date   2026/08/19
		 */
		class WeaponInventory
		{
		public:
			/* コンストラクタとデストラクタ。*/
			WeaponInventory() = default;
			virtual ~WeaponInventory() = default;


		public:
			/**
			 * @brief 武器を種類指定で所持リストに追加する。
			 * @param enType 追加する武器の種類。
			 */
			void AddWeapon(EnWeaponType enType);

			/**
			 * @brief 毎フレーム呼ぶ更新処理(現在武器のクールタイム等を進める)。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void Update(float fDeltaTime);

			/**
			 * @brief 現在の武器で発射を試みる。
			 * @param vPosition  発射位置。
			 * @param vDirection 発射方向。
			 * @return 発射できた場合はtrue。
			 */
			bool Fire(const Vector3& vPosition, const Vector3& vDirection);

			/**
			 * @brief 現在の武器をリロードする。
			 */
			void Reload();

			/**
			 * @brief 次の武器に切り替える(リスト末尾なら先頭に戻る)。
			 */
			void SwitchNext();

			/**
			 * @brief 前の武器に切り替える(リスト先頭なら末尾に戻る)。
			 */
			void SwitchPrev();

			/**
			 * @brief 指定した区分の武器へ直接持ち替える(数字キーでの切り替えに使う)。
			 *        持っていない区分や、すでに持っている武器を指定した場合は何もしない。
			 * @param enSlot 持ち替えたい区分。
			 */
			void SwitchToSlot(EnWeaponSlot enSlot);

			/**
			 * @brief 所持している武器へ予備弾を補給する。
			 * @param iAmount 武器1丁あたりに補給する数。
			 * @return 1発でも補給できたら true。
			 */
			bool AddReserveAmmoToAll(int iAmount);


		/* ゲッター。*/
		public:
			/**
			 * @brief 現在の武器を取得する(UI表示等で使用)。
			 * @return 現在の武器。所持していない場合はnullptr。
			 */
			Weapon* GetCurrentWeapon();


		private:
			std::vector<Weapon>	vecWeapons_;		//! 所持武器リスト。
			int					iCurrentIndex_ = 0;	//! 現在選択中のインデックス。
		};
	}
}
