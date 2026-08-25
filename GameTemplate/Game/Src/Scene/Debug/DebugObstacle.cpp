#include "stdafx.h"
#include "DebugObstacle.h"

namespace
{
	const char* sWallModelPath_ = "Assets/modelData/Block.tkm";
}

namespace nsApp
{
	namespace nsScene
	{
		DebugObstacle::~DebugObstacle()
		{
			stStaticObject_.Release();
		}


		void DebugObstacle::SetTransform(const Vector3& vPos, const Vector3& vSize)
		{
			vPosition_ = vPos;
			vSize_ = vSize;

			if (IsStart())
				ApplyTransform();
		}


		bool DebugObstacle::Start()
		{
			CreateModel();
			ApplyTransform();
			return true;
		}


		void DebugObstacle::ApplyTransform()
		{
			/* 見た目を先に確定する。*/
			stModel_.SetPosition(vPosition_);
			stModel_.SetScale(CalcModelScale());	
			stModel_.Update();

			/* 見た目のワールド行列から静的コライダを作る（既存API）。*/
			stStaticObject_.Release();
			stStaticObject_.CreateFromModel(
				stModel_.GetModel(),
				stModel_.GetModel().GetWorldMatrix()
			);
		}


		Vector3 DebugObstacle::CalcModelScale() const
		{
			const float fInv = 1.0f / fModelBaseSize_;
			return Vector3(vSize_.x * fInv, vSize_.y * fInv, vSize_.z * fInv);
		}


		void DebugObstacle::CreateModel()
		{
			stModel_.Init(sWallModelPath_, nullptr, 0, enModelUpAxisY);
		}


		void DebugObstacle::Render(RenderContext& rc)
		{
			stModel_.Draw(rc);
		}
	}
}