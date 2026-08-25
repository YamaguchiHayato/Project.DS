#include "stdafx.h"
#include "DebugEnemyScene.h"
#include "Src/Scene/Debug/DebugPlayer.h"
#include "Src/Scene/GameFlow.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Scene/Debug/DebugObstacle.h"

namespace
{
	const float fCameraBackDistance_ = 200.0f; //! プレイヤーからカメラまでの距離。
	const float fCameraHeight_ = 100.0f; //! カメラの高さ。
	const float fCameraTargetHeight_ = 50.0f; //! 注視点の高さ。
	const Vector3 vDebugFontPos_ = { -450.0f, 450.0f, 0.0f }; //! 敵Debug表示の位置。
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

			/* 生成した障害物を破棄する。*/
			if (pObstacle_ != nullptr)
			{
				DeleteGO(pObstacle_);
				pObstacle_ = nullptr;
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

			/* 障害物を生成する。*/
			pObstacle_ = NewGO<DebugObstacle>(0, "debugObstacle");
			pObstacle_->SetTransform(
				Vector3(0.0f, 0.0f, 250.0f),
				Vector3(750.0f, 750.0f, 750.0f)
			);

			/* Debug構成でコライダ枠を表示する。*/
			nsK2EngineLow::PhysicsWorld::GetInstance()->EnableDrawDebugWireFrame();

			/* 遷移直後に押しっぱなしのキーを新規入力扱いにしない。*/
			bWasPressEsc_ = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

			/* デバッグ表示用のフォントを初期化する。*/
			stDebugFont_.SetPosition(vDebugFontPos_);
			stDebugFont_.SetScale(1.0f);
			stDebugFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);

			return true;
		}


		void DebugEnemyScene::Update()
		{
			/* 今フレームのキー状態を取る。*/
			const bool bPressEsc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
			const bool bPressT = (GetAsyncKeyState('T') & 0x8000) != 0;

			/* Tを押した瞬間だけ敵へDebugダメージ。*/
			if (bPressT && !bWasPressT_)
				DebugDamageEnemy();

			/* ESCを押した瞬間だけDebug選択へ戻る。*/
			if (bPressEsc && !bWasPressEsc_)
				pGameFlow_->ChangeScene(EnSceneID::Debug);

			/* 次フレーム判定用に今の状態を残す。*/
			bWasPressEsc_ = bPressEsc;
			bWasPressT_ = bPressT;

			/* デバッグ表示を更新する。*/
			UpdateDebugFont();

			/* プレイヤー追従カメラを更新する。*/
			UpdateFollowCamera();
		}


		void DebugEnemyScene::Render(RenderContext& rc)
		{
			/* デバッグ情報を描画する。*/
			stDebugFont_.Draw(rc);
		}


		void DebugEnemyScene::UpdateDebugFont()
		{
			/* 敵が無ければ処理しない。*/
			if (pCommonEnemy_ == nullptr)
				return;

			/* 敵のステート名、視線の有無、HPを表示する。*/
			const wchar_t* sVisible = pCommonEnemy_->IsTargetVisible() ? L"YES" : L"NO";

			/* デバッグ表示用の文字列を作る。*/	
			swprintf_s(
				aDebugText_,
				L"STATE: %s  VISIBLE: %s  HP: %d  [T]",
				pCommonEnemy_->GetCurrentStateName(),
				sVisible,
				pCommonEnemy_->GetCurrentHP()
			);

			/* デバッグ表示用のフォントに文字列をセットする。*/
			stDebugFont_.SetText(aDebugText_);
		}
	

		void DebugEnemyScene::DebugDamageEnemy()
		{
			/* 敵が無ければ処理しない。*/
			if (pCommonEnemy_ == nullptr)
				return;
			/* Debug用ダメージを与える。*/
			pCommonEnemy_->ApplyDamage(50);
		}


		void DebugEnemyScene::InitCamera()
		{
			/* Near/Farだけ設定する。位置はUpdateFollowCameraで毎フレーム更新。*/
			g_camera3D->SetNear(1.0f);
			g_camera3D->SetFar(10000.0f);
		}


		void DebugEnemyScene::UpdateFollowCamera()
		{
			if (pDummyPlayer_ == nullptr)
				return;

			const Vector3 vPlayerPos = pDummyPlayer_->GetPosition();

			g_camera3D->SetPosition(vPlayerPos + Vector3(0.0f, 120.0f, -280.0f));
			g_camera3D->SetTarget(vPlayerPos + Vector3(0.0f, 50.0f, 0.0f));
			g_camera3D->Update();
		}	
	}
}