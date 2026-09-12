#include "stdafx.h"
#include "Pickup.h"
#include "Src/System/GamePause.h"
#include "Src/System/ModelBounds.h"
#include "Src/Data/WeaponStatusTable.h"

namespace
{
	const char* sPickupModelPath_ = "Assets/modelData/preset/VolumePointLight.tkm";	//! 仮モデル(発光球)。
	const float kPickupRange = 120.0f;		//! 拾える距離。
	const float kPickupHeight = 40.0f;		//! 目立つよう少し浮かせる高さ。
	const float kSpinSpeed = 1.5f;			//! 回転の速さ(ラジアン/秒)。

	/**
	 * @brief 物資の種類ごとの表示サイズを返す。
	 * @param enType 物資の種類。
	 * @return 表示サイズ。
	 */
	float GetPickupScale(nsApp::nsItem::EnPickupType enType)
	{
		/* 弾薬は数が多いので小さめ、回復は目立つよう大きめにする。*/
		switch (enType)
		{
		case nsApp::nsItem::EnPickupType::Medkit:
			return 26.0f;

		case nsApp::nsItem::EnPickupType::PipeBomb:
			return 20.0f;

		case nsApp::nsItem::EnPickupType::Pills:
		case nsApp::nsItem::EnPickupType::Adrenaline:
			/* 瓶や注射器なので小さめ。*/
			return 16.0f;

		default:
			return 22.0f;
		}
	}
}

namespace nsApp
{
	namespace nsItem
	{
		bool Pickup::Start()
		{
			if (enType_ == EnPickupType::Weapon)
				InitWeaponModel();
			else
				InitItemModel();

			/* 地面に埋まって見えなくならないよう、少し浮かせた位置へ置く。*/
			ApplyModelTransform();

			return true;
		}


		void Pickup::InitItemModel()
		{
			/* 仮モデル(発光球)。種類で大きさだけ変える。*/
			const float fScale = GetPickupScale(enType_);
			stModel_.Init(sPickupModelPath_, nullptr, 0, enModelUpAxisY);
			stModel_.SetScale(Vector3(fScale, fScale, fScale));
			vModelOffset_ = Vector3::Zero;
		}


		void Pickup::InitWeaponModel()
		{
			/* 落ちている銃は、その銃のモデルをそのまま使う(手に持たせるときと同じ実寸)。*/
			const nsWeapon::WeaponStatus& stStatus = nsData::WeaponStatusTable::Get(enWeaponType_);
			stModel_.Init(stStatus.pModelPath_, nullptr, 0, enModelUpAxisZ);

			/*
			 * 銃のモデルは原点が中心に無く、基準の大きさも銃ごとに違う。
			 * 頂点から測って、手に持たせたときの長さ(handLength)に合わせ、中心が置き場所に来るようにする。
			 */
			const nsSystem::ModelBounds stBounds = nsSystem::MeasureModelBounds(stModel_.GetModel());
			const float fLongest = stBounds.GetLongestEdge();
			const float fScale = (fLongest > 0.0001f) ? (stStatus.fHandLength_ / fLongest) : 1.0f;

			stModel_.SetScale(Vector3(fScale, fScale, fScale));
			vModelOffset_ = stBounds.GetCenter() * fScale;
		}


		void Pickup::ApplyModelTransform()
		{
			/* 回転は落ちているのが分かるよう、その場で回し続けているぶん。*/
			Quaternion qRotation;
			qRotation.SetRotationY(fSpinAngle_);

			/* 原点のズレは回転に合わせて回してから打ち消す。*/
			Vector3 vOffset = vModelOffset_;
			qRotation.Apply(vOffset);

			Vector3 vViewPos = vPosition_;
			vViewPos.y += kPickupHeight;
			vViewPos -= vOffset;

			stModel_.SetPosition(vViewPos);
			stModel_.SetRotation(qRotation);
			stModel_.Update();
		}


		void Pickup::Update()
		{
			/* ポーズ中は動かさない。*/
			if (nsSystem::IsGamePaused())
				return;

			/* 落ちているのが分かるよう、その場で回し続ける。*/
			fSpinAngle_ += kSpinSpeed * g_gameTime->GetFrameDeltaTime();

			/* 進めた角度をモデルへ反映する。*/
			ApplyModelTransform();
		}


		void Pickup::Render(RenderContext& rc)
		{
			/* 落ちている物資を描画する。*/
			stModel_.Draw(rc);
		}


		bool Pickup::IsInRange(const Vector3& vPlayerPosition) const
		{
			/* 高さは見ずに、水平の距離だけで判定する。*/
			Vector3 vDiff = vPosition_ - vPlayerPosition;
			vDiff.y = 0.0f;

			/* 拾える距離の内側にいるかを返す。*/
			return vDiff.Length() <= kPickupRange;
		}
	}
}
