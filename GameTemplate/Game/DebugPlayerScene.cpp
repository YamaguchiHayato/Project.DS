#include "stdafx.h"
#include "DebugPlayerScene.h"
#include "Player.h"

namespace
{
	const float fEyeHeight_ = 160.0f;							//! 目(カメラ)の高さ。Player側の kEyeHeight と合わせる。

	const char* sGroundModelPath_ = "Assets/modelData/ground.tkm";	//! 地面モデル。
	const float fGroundScale_ = 200.0f;								//! 地面の拡大率(目印用に大きく敷く)。
}

namespace nsApp
{
	namespace nsScene
	{
		DebugPlayerScene::~DebugPlayerScene()
		{
			/* 生成したプレイヤーを破棄する。*/
			if (pPlayer_ != nullptr)
			{
				DeleteGO(pPlayer_);
				pPlayer_ = nullptr;
			}
		}


		bool DebugPlayerScene::Start()
		{
			/* カメラを初期化する。*/
			InitCamera();

			/* 地面(目印)を読み込んで大きく敷く。*/
			stGroundModel_.Init(sGroundModelPath_, nullptr, 0, enModelUpAxisY);
			stGroundModel_.SetScale(Vector3(fGroundScale_, fGroundScale_, fGroundScale_));
			stGroundModel_.SetPosition(Vector3(0.0f, 0.0f, 0.0f));
			stGroundModel_.Update();

			/* プレイヤーを生成する。*/
			pPlayer_ = NewGO<nsActor::Player>(0, "player");

			/* 生成直後のプレイヤー位置にカメラを合わせる。*/
			UpdateCamera();
			return true;
		}


		void DebugPlayerScene::Update()
		{
			/* カメラをプレイヤーに追従させる。*/
			UpdateCamera();
		}


		void DebugPlayerScene::Render(RenderContext& rc)
			{
				/* 地面を描画する(カメラの回転が目で分かるようにする)。*/
				stGroundModel_.Draw(rc);
			}


			void DebugPlayerScene::InitCamera()
		{
			/* ニアクリップ・ファークリップを設定する。*/
			g_camera3D->SetNear(1.0f);
			g_camera3D->SetFar(10000.0f);
		}


		void DebugPlayerScene::UpdateCamera()
		{
			/* プレイヤーが無ければ何もしない。*/
			if (pPlayer_ == nullptr)
				return;

			/* プレイヤーの旋回角と位置から、目の位置と視線方向を決める(一人称)。*/
			const float fYaw = pPlayer_->GetCameraYaw();
			const Vector3& vPlayerPos = pPlayer_->GetPosition();

			/* 目の位置(プレイヤー座標＋目線の高さ)。*/
			Vector3 vEyePos = vPlayerPos;
			vEyePos.y += fEyeHeight_;

			/* 視線方向(旋回角の水平前方)。*/
			const Vector3 vLook = { sinf(fYaw), 0.0f, cosf(fYaw) };

			g_camera3D->SetPosition(vEyePos);
			g_camera3D->SetTarget(vEyePos + vLook * 100.0f);
			g_camera3D->Update();
		}
	}
}
