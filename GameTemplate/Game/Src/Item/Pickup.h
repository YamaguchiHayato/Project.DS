#pragma once
#include "Weapon.h"

namespace nsApp
{
	namespace nsItem
	{
		/**
		 * @enum  EnPickupType
		 * @brief 拾える物資の種類。
		 */
		enum class EnPickupType : uint8_t
		{
			Ammo,		//! 弾薬の山。予備弾を満タンにする。触れても無くならない。
			Medkit,		//! 回復アイテム。
			PipeBomb,	//! パイプ爆弾(投擲アイテム)。
			Pills,		//! 鎮痛剤(即効アイテム)。
			Adrenaline,	//! アドレナリン(即効アイテム)。
			Weapon,		//! 銃。拾うと同じ区分の手持ちと入れ替わる(どの銃かは enWeaponType_)。
		};

		/**
		 * @file   Pickup.h
		 * @brief  マップに置かれた、拾える物資。
		 *         プレイヤーが近づいて使用キーを押すと拾われて消える。
		 *         種類ごとに何が補給されるかは Player 側が決める。
		 * @author Izumida Kiryu
		 * @date   2026/08/27
		 */
		class Pickup : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			Pickup() = default;
			virtual ~Pickup() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;

			/**
			 * @brief 置く場所と種類を設定する。NewGOした直後(Startより前)に呼ぶ。
			 * @param enType 物資の種類。
			 * @param vPosition 置く場所。
			 */
			void Setup(EnPickupType enType, const Vector3& vPosition)
			{
				enType_ = enType;
				vPosition_ = vPosition;
			}

			/**
			 * @brief 落ちている銃として置く場所と銃の種類を設定する。NewGOした直後(Startより前)に呼ぶ。
			 *        見た目はその銃のモデルになる。
			 * @param enWeaponType 銃の種類。
			 * @param vPosition    置く場所。
			 */
			void SetupWeapon(nsWeapon::EnWeaponType enWeaponType, const Vector3& vPosition)
			{
				enType_ = EnPickupType::Weapon;
				enWeaponType_ = enWeaponType;
				vPosition_ = vPosition;
			}

			//! 物資の種類。
			inline EnPickupType GetType() const { return enType_; }

			//! 銃の種類(物資の種類が Weapon のときだけ意味を持つ)。
			inline nsWeapon::EnWeaponType GetWeaponType() const { return enWeaponType_; }

			//! 置かれている場所。
			inline const Vector3& GetPosition() const { return vPosition_; }

			//! 拾える距離の内側にいるか。
			bool IsInRange(const Vector3& vPlayerPosition) const;


		private:
			/**
			 * @brief 物資(弾薬・回復など)の仮モデルを用意する。
			 */
			void InitItemModel();

			/**
			 * @brief 落ちている銃のモデルを用意する。その銃のモデルを実寸で置く。
			 */
			void InitWeaponModel();

			/**
			 * @brief 位置・回転・原点ズレの打ち消しをモデルへ反映する。
			 */
			void ApplyModelTransform();


		private:
			ModelRender stModel_;					//! 見た目のモデル。
			EnPickupType enType_ = EnPickupType::Ammo;	//! 物資の種類。
			nsWeapon::EnWeaponType enWeaponType_ = nsWeapon::EnWeaponType::Handgun;	//! 銃の種類(Weapon のときだけ使う)。
			Vector3 vModelOffset_ = Vector3::Zero;	//! モデル原点のズレを打ち消す量(銃のモデルは原点が中心に無い)。
			Vector3 vPosition_ = Vector3::Zero;		//! 置かれている場所。
			float fSpinAngle_ = 0.0f;				//! 目立たせるための回転角(ラジアン)。
		};
	}
}
