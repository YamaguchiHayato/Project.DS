#include "stdafx.h"
#include "Tracer.h"

namespace
{
	const char*	sTracerModelPath_ = "Assets/modelData/preset/VolumePointLight.tkm";	//! トレーサーのモデル(仮の球を細長く伸ばして線にする)。
	const float	fTracerThickness_ = 3.0f;	//! トレーサーの太さ。
	const float	fTracerLifeTime_ = 0.05f;	//! 表示時間(秒)。一瞬だけ見せる。
}

namespace nsApp
{
	namespace nsWeapon
	{
		bool Tracer::Start()
		{
			/* 始点→終点のベクトルから、長さと向き(正規化)と中点を求める。*/
			Vector3 vDiff = vEnd_ - vStart_;
			const float fLength = sqrtf(vDiff.x * vDiff.x + vDiff.y * vDiff.y + vDiff.z * vDiff.z);

			Vector3 vDir = vDiff;
			vDir.Normalize();

			Vector3 vMid = vStart_ + vEnd_;
			vMid *= 0.5f;

			/* 中点に置き、モデルのローカルZ軸を射線方向(ヨー＋ピッチの3D)へ向ける。*/
			stModel_.Init(sTracerModelPath_, nullptr, 0, enModelUpAxisY);
			stModel_.SetPosition(vMid);

			Quaternion qRotation;
			qRotation.SetRotationY(atan2f(vDir.x, vDir.z));	// まず水平のヨー。
			{
				/* 上下のピッチをローカル軸で後乗せ(細い線を射線に沿わせる)。asinfのNaN回避でクランプ。*/
				float fSin = vDir.y;
				if (fSin > 1.0f) fSin = 1.0f;
				else if (fSin < -1.0f) fSin = -1.0f;
				qRotation.AddRotationX(-asinf(fSin));
			}
			stModel_.SetRotation(qRotation);

			/* 太さは細く、長さ方向(Z)は区間の長さぶん伸ばして1本の線にする。*/
			stModel_.SetScale(Vector3(fTracerThickness_, fTracerThickness_, fLength));
			stModel_.Update();

			fLifeTimer_ = fTracerLifeTime_;
			return true;
		}


		void Tracer::Update()
		{
			/* 表示時間が尽きたら自分を消す。*/
			fLifeTimer_ -= g_gameTime->GetFrameDeltaTime();
			if (fLifeTimer_ <= 0.0f)
			{
				DeleteGO(this);
				return;
			}
		}


		void Tracer::Render(RenderContext& rc)
		{
			/* トレーサーを描画する。*/
			stModel_.Draw(rc);
		}


		void Tracer::Setup(const Vector3& vStart, const Vector3& vEnd)
		{
			vStart_ = vStart;
			vEnd_ = vEnd;
		}
	}
}
