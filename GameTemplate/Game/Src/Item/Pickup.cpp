#include "stdafx.h"
#include "Pickup.h"
#include "Src/System/GamePause.h"

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

		case nsApp::nsItem::EnPickupType::Grenade:
			return 20.0f;

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
			/* 仮モデルを、少し浮かせた位置に置く。*/
			const float fScale = GetPickupScale(enType_);
			stModel_.Init(sPickupModelPath_, nullptr, 0, enModelUpAxisY);
			stModel_.SetScale(Vector3(fScale, fScale, fScale));

			/* 地面に埋まって見えなくならないよう、少し浮かせた位置へ置く。*/
			Vector3 vViewPos = vPosition_;
			vViewPos.y += kPickupHeight;
			stModel_.SetPosition(vViewPos);
			stModel_.Update();

			return true;
		}


		void Pickup::Update()
		{
			/* ポーズ中は動かさない。*/
			if (nsSystem::IsGamePaused())
				return;

			/* 落ちているのが分かるよう、その場で回し続ける。*/
			fSpinAngle_ += kSpinSpeed * g_gameTime->GetFrameDeltaTime();

			/* 進めた角度をモデルへ反映する。*/
			Quaternion qRotation;
			qRotation.SetRotationY(fSpinAngle_);
			stModel_.SetRotation(qRotation);
			stModel_.Update();
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
