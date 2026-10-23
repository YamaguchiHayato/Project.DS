#include "k2EngineLowPreCompile.h"
#include "Flashlight.h"

namespace
{
	const int iToggleKey_ = 'F';	//! 点け消しするキー。
	const int iSpotSlot_ = 9;		//! 使うスポットライトの番号。JSONでは使わない。
	const float fPi_ = 3.14159265f;
}

void nsK2EngineLow::Flashlight::Update(SceneLight& light)
{
	/* 押した瞬間だけ切り替える。装備していなければ、キーは効かない。*/
	const bool bPressKey = (GetAsyncKeyState(iToggleKey_) & 0x8000) != 0;
	if (bIsEquipped_ && bPressKey && !bWasPressKey_)
		bIsOn_ = !bIsOn_;
	bWasPressKey_ = bPressKey;

	SpotLight& stSpot = light.spotLig[iSpotSlot_];

	/* 消灯中は、範囲0にして、シェーダーに飛ばさせる。*/
	if (!bIsEquipped_ || !bIsOn_)
	{
		stSpot.m_range = 0.0f;
		stSpot.m_color = Vector3::Zero;
		return;
	}

	/* カメラの位置から、視線の先へ向ける。*/
	Vector3 vDir = g_camera3D->GetTarget() - g_camera3D->GetPosition();
	vDir.Normalize();

	stSpot.m_position = g_camera3D->GetPosition();
	stSpot.m_direction = vDir;
	stSpot.m_color = vColor_;
	stSpot.m_range = fRange_;
	stSpot.m_angle = fHalfAngleDeg_ * fPi_ / 180.0f;	//! シェーダーはラジアンで受け取る。

	/* 確認用: 30フレームに1回、位置と向きを出す。確認できたら消す。*/
	static int s_iLogCount = 0;
	if (++s_iLogCount % 30 == 0)
	{
		char buf[160];
		sprintf_s(buf, "flashlight pos(%.0f,%.0f,%.0f) dir(%.2f,%.2f,%.2f)\n",
			stSpot.m_position.x, stSpot.m_position.y, stSpot.m_position.z,
			stSpot.m_direction.x, stSpot.m_direction.y, stSpot.m_direction.z);
		OutputDebugStringA(buf);
	}
}