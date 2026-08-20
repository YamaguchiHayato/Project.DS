#include "stdafx.h"
#include "DebugEnemyScene.h"
#include "DebugPlayer.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"

namespace
{
	const Vector3 vCameraPosition_ = { 0.0f, 120.0f, -280.0f };	//! カメラ位置。
	const Vector3 vCameraTarget_ = { 0.0f, 50.0f, 0.0f };		//! カメラ注視点。
}

namespace nsApp
{
	namespace nsScene
	{
		DebugEnemyScene::~DebugEnemyScene()
		{
			/* 生成した仮プレイヤーを破棄する。*/
			if (pDummyPlayer_ != nullptr)
			{
				DeleteGO(pDummyPlayer_);
				pDummyPlayer_ = nullptr;
			}

			/* 生成した雑魚敵を破棄する。*/
			if (pCommonEnemy_ != nullptr)
			{
				DeleteGO(pCommonEnemy_);
				pCommonEnemy_ = nullptr;
			}
		}


		bool DebugEnemyScene::Start()
		{
			/* カメラをセットする。*/
			InitCamera();

			/* 仮プレイヤーを生成する。*/
			pDummyPlayer_ = NewGO<nsActor::DummyPlayer>(0, "dummyPlayer");

			/* 雑魚敵を生成する。*/
			pCommonEnemy_ = NewGO<nsActor::CommonEnemy>(0, "commonEnemy");
			pCommonEnemy_->SetTarget(pDummyPlayer_);
			return true;
		}


		void DebugEnemyScene::Update()
		{}


		void DebugEnemyScene::InitCamera()
		{
			/* 2体が見える位置に固定カメラを置く。*/
			g_camera3D->SetNear(1.0f);
			g_camera3D->SetFar(10000.0f);
			g_camera3D->SetPosition(vCameraPosition_);
			g_camera3D->SetTarget(vCameraTarget_);
			g_camera3D->Update();
		}
	}
}