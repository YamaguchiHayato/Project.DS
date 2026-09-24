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
			 * @brief 同じ区分(メイン/サブ)の手持ちの武器を、別の武器と入れ替えて構える。
			 *        その区分を持っていなければ追加になる。落ちている銃を拾ったときに使う。
			 * @param enNewType   新しく持つ武器の種類。
			 * @param enOutOldType 手放した武器の種類を受け取る(追加になった場合は enNewType が入る)。
			 * @return 入れ替え(または追加)したら true。すでに同じ種類を持っていれば false。
			 */
			bool ReplaceWeapon(EnWeaponType enNewType, EnWeaponType& enOutOldType);

			/**
			 * @brief 毎フレーム呼ぶ更新処理(現在武器のクールタイム等を進める)。
			 * @param fDeltaTime       1フレームの経過時間(秒)。
			 * @param fActionSpeedRate リロードと構えの速さの倍率(アドレナリン中は1より大きくなる)。
			 */
			void Update(float fDeltaTime, float fActionSpeedRate = 1.0f);

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
			 * @brief 現在の武器のリロードを中断する(突き飛ばしなどで手を使ったとき)。
			 */
			void CancelReload();

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

			/**
			 * @brief 所持している武器の予備弾を全て上限まで満たす(弾薬の山に触れたとき。本家と同じく満タンになる)。
			 * @return 1発でも補給できたら true。
			 */
			bool RefillReserveAmmoToAll();


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
