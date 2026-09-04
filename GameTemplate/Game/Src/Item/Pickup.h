#pragma once

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
			Ammo,		//! 予備弾薬。
			Medkit,		//! 回復アイテム。
			Grenade,	//! 投擲アイテム。
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

			//! 物資の種類。
			inline EnPickupType GetType() const { return enType_; }

			//! 置かれている場所。
			inline const Vector3& GetPosition() const { return vPosition_; }

			//! 拾える距離の内側にいるか。
			bool IsInRange(const Vector3& vPlayerPosition) const;


		private:
			ModelRender stModel_;					//! 見た目のモデル。
			EnPickupType enType_ = EnPickupType::Ammo;	//! 物資の種類。
			Vector3 vPosition_ = Vector3::Zero;		//! 置かれている場所。
			float fSpinAngle_ = 0.0f;				//! 目立たせるための回転角(ラジアン)。
		};
	}
}
