#pragma once
#include "Weapon.h"

namespace nsApp
{
	namespace nsWeapon
	{
		/**
		 * @file   WeaponInventory.h
		 * @brief  所持している武器を「区分(メイン/サブ)ごとの枠」で管理し、
		 *         現在の武器の切り替え・発射をまとめて扱うクラス。
		 *         枠は EnWeaponSlot の添字そのもので、1区分につき1丁だけ持てる。
		 *         Player/Enemyどちらからも使える。
		 * @note   どの銃がどの枠へ入るかは基本的に武器データ(weapon.json の slot)が決めるが、
		 *         サブ武器にかぎり、同じ銃をもう1丁拾ったときだけメイン枠へ回せる(二丁持ち)。
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
			 * @brief 武器を種類指定で、その武器が本来入る枠へ入れる(開始時の装備を並べるのに使う)。
			 *        すでに何か入っている枠なら上書きする。
			 * @param enType 追加する武器の種類。
			 */
			void AddWeapon(EnWeaponType enType);

			/**
			 * @brief 落ちている銃を拾って、枠の中身と入れ替えて構える。
			 *        入れ先の枠は DecideSlot が決める(空いていればそこへ、同じ銃を持っていればもう一方の枠へ)。
			 * @param enNewType    新しく持つ武器の種類。
			 * @param enOutOldType 手放した武器の種類を受け取る。
			 *                     空いていた枠へ入れて何も手放さなかった場合は EnWeaponType::Num が入る。
			 * @return 拾えたら true。入れ先が無ければ false(物資はその場に残す)。
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
			 * @brief 次の枠の武器に切り替える(末尾なら先頭に戻る)。持っていない枠は飛ばす。
			 */
			void SwitchNext();

			/**
			 * @brief 前の枠の武器に切り替える(先頭なら末尾に戻る)。持っていない枠は飛ばす。
			 */
			void SwitchPrev();

			/**
			 * @brief 指定した区分の武器へ直接持ち替える(数字キーでの切り替えに使う)。
			 *        持っていない区分や、すでに構えている区分を指定した場合は何もしない。
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

			/**
			 * @brief 指定した枠の武器を取得する(二丁持ちの成立判定や、左手の銃を見るのに使う)。
			 * @param enSlot 見たい区分。
			 * @return その枠の武器。持っていなければ nullptr。
			 */
			Weapon* GetWeapon(EnWeaponSlot enSlot);

			/**
			 * @brief 指定した枠に武器が入っているか。
			 * @param enSlot 見たい区分。
			 * @return 入っていれば true。
			 */
			bool HasWeapon(EnWeaponSlot enSlot) const;

			/**
			 * @brief いま構えている区分を取得する。
			 * @return 構えている区分。
			 */
			inline EnWeaponSlot GetCurrentSlot() const
			{
				return enCurrentSlot_;
			}


		private:
			/**
			 * @brief 拾った銃をどの枠へ入れるかを決める。
			 * @param enNewType 拾った武器の種類。
			 * @return 入れ先の区分。入れ先が無ければ EnWeaponSlot::Num。
			 */
			EnWeaponSlot DecideSlot(EnWeaponType enNewType) const;

			/**
			 * @brief 指定した枠を構える(構える動作を始める。リロード中だった場合は中断される)。
			 * @param enSlot 構える区分。
			 */
			void EquipSlot(EnWeaponSlot enSlot);

			/**
			 * @brief 1丁も持っていないか。
			 * @return 1丁も持っていなければ true。
			 */
			bool IsEmpty() const;


		private:
			static constexpr int iSlotNum_ = static_cast<int>(EnWeaponSlot::Num);	//! 所持枠の数。

			/*
			 * 枠ごとの武器。bHasWeapon_ が false の枠は中身が未初期化なので触らないこと。
			 * 出し入れは必ず上のメソッド経由にして、この約束をここに閉じ込める。
			 */
			Weapon			aWeapons_[iSlotNum_];				//! 枠ごとの所持武器。
			bool			abHasWeapon_[iSlotNum_] = {};		//! その枠に武器が入っているか。
			EnWeaponSlot	enCurrentSlot_ = EnWeaponSlot::Sub;	//! いま構えている区分。
		};
	}
}
