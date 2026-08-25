#include "stdafx.h"
#include "HitEffect.h"

namespace
{
	const char*	sEffectModelPath_ = "Assets/modelData/preset/VolumePointLight.tkm";	//! 発光球(Tracerと共通)。
}

namespace nsApp
{
	namespace nsEffect
	{
		bool HitEffect::Start()
		{
			/* 発光球を開始サイズで置く。*/
			stModel_.Init(sEffectModelPath_, nullptr, 0, enModelUpAxisY);
			stModel_.SetPosition(vPos_);
			stModel_.SetScale(Vector3(fStartScale_, fStartScale_, fStartScale_));
			stModel_.Update();

			fTimer_ = 0.0f;
			return true;
		}


		void HitEffect::Update()
		{
			fTimer_ += g_gameTime->GetFrameDeltaTime();

			/* 寿命が尽きたら自分を消す。*/
			if (fTimer_ >= fLife_)
			{
				DeleteGO(this);
				return;
			}

			/* 経過に応じてサイズを開始→終了へ補間する。*/
			const float fRate = (fLife_ > 0.0f) ? (fTimer_ / fLife_) : 1.0f;
			const float fScale = fStartScale_ + (fEndScale_ - fStartScale_) * fRate;
			stModel_.SetScale(Vector3(fScale, fScale, fScale));
			stModel_.Update();
		}


		void HitEffect::Render(RenderContext& rc)
		{
			/* 発光球を描画する。*/
			stModel_.Draw(rc);
		}


		void HitEffect::Setup(const Vector3& vPos, float fStartScale, float fEndScale, float fLifeSec)
		{
			vPos_ = vPos;
			fStartScale_ = fStartScale;
			fEndScale_ = fEndScale;
			fLife_ = fLifeSec;
		}
	}
}
