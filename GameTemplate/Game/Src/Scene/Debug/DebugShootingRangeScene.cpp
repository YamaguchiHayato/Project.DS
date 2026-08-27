#include "stdafx.h"
#include "DebugShootingRangeScene.h"
#include "Src/Scene/GameFlow.h"
#include "Player.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"

namespace
{
	const int iTargetEnemyCount_ = 12;
	const float fEyeHeight_ = 160.0f;
	const char* sStageModelPath_ = "Assets/modelData/stage/DebugStage/demoStage.tkm";
	const Vector3 vHintFontPos_ = { -450.0f, 450.0f, 0.0f };
	const float fStageScale_ = 150.0f;

	/* 手前中央。敵とは離す。*/
	const Vector3 vPlayerSpawn_ = { 0.0f, 300.0f, -600.0f };

	const Vector3 aTargetPositions_[12] =
	{
		/* 手前〜中（左右に開く） */
		{ -900.0f, 300.0f,  -50.0f },
		{  900.0f, 300.0f,    0.0f },
		{ -400.0f, 300.0f,  200.0f },
		{  450.0f, 300.0f,  280.0f },

		/* 中盤（広く散らす） */
		{ -1100.0f, 300.0f,  700.0f },
		{     0.0f, 300.0f,  650.0f },
		{  1100.0f, 300.0f,  720.0f },
		{  -550.0f, 300.0f,  900.0f },
		{   600.0f, 300.0f,  950.0f },

		/* 奥（青の構造物側） */
		{ -800.0f, 300.0f, 1400.0f },
		{  200.0f, 300.0f, 1550.0f },
		{  850.0f, 300.0f, 1350.0f },
	};
}


namespace nsApp
{
	namespace nsScene
	{
		DebugShootingRangeScene::~DebugShootingRangeScene()
		{
			/* 倒済みは FindGOs に出ないので二重破棄しない。*/
			for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
				DeleteGO(pEnemy);

			for (int i = 0; i < iTargetEnemyCount_; ++i)
				aTargetEnemies_[i] = nullptr;

			if (pPlayer_ != nullptr)
			{
				DeleteGO(pPlayer_);
				pPlayer_ = nullptr;
			}
		}


		bool DebugShootingRangeScene::Start()
		{
			InitCamera();

			/* InGameの床と同じ手順。demoStage に Physics を付ける。*/
			stGroundModel_.Init(sStageModelPath_, nullptr, 0, enModelUpAxisZ);
			stGroundModel_.SetScale(Vector3(fStageScale_, fStageScale_, fStageScale_));
			stGroundModel_.SetPosition(Vector3::Zero);
			stGroundModel_.SetRotation(Quaternion::Identity);
			stGroundModel_.Update();

			/* PhysicsStaticObject を作り直す。 */
			stGroundCollider_.Release();
			stGroundCollider_.CreateFromModel(stGroundModel_.GetModel(),stGroundModel_.GetModel().GetWorldMatrix());

			/* プレイヤーを生成する。 */
			pPlayer_ = NewGO<nsActor::Player>(0, "player");
			pPlayer_->SetPosition(vPlayerSpawn_);

			/* 的役の敵を奥に配置する。 */
			SpawnTargetEnemies();

			/* ヒントを描画する。 */
			stHintFont_.SetPosition(vHintFontPos_);
			stHintFont_.SetScale(1.0f);
			stHintFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stHintFont_.SetText(L"SHOOTING RANGE  ESC BACK");

			/* 前フレームのESC状態を取る。 */
			bWasPressEsc_ = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
			UpdateCamera();
			return true;
		}


		void DebugShootingRangeScene::Update()
		{
			/* 今フレームのESC状態を取る。*/
			const bool bPressEsc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

			/* ESCを押した瞬間だけDebug選択へ戻る。*/
			if (bPressEsc && !bWasPressEsc_)
				pGameFlow_->ChangeScene(EnSceneID::Debug);

			/* 次フレーム判定用に今の状態を残す。*/
			bWasPressEsc_ = bPressEsc;

			/* 一人称カメラをプレイヤーへ追従させる。*/
			UpdateCamera();
		}


		void DebugShootingRangeScene::Render(RenderContext& rc)
		{
			/* 地面を描画する。*/
			stGroundModel_.Draw(rc);

			/* ヒントを描画する。*/
			stHintFont_.Draw(rc);
		}


		void DebugShootingRangeScene::InitCamera()
		{
			/* ニアクリップ・ファークリップを設定する。*/
			g_camera3D->SetNear(1.0f);
			g_camera3D->SetFar(10000.0f);
		}


		void DebugShootingRangeScene::UpdateCamera()
		{
			/* プレイヤーが無ければ何もしない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 目の位置(プレイヤー座標＋目線の高さ)。*/
			const Vector3& vPlayerPos = pPlayer_->GetPosition();
			Vector3 vEyePos = vPlayerPos;
			vEyePos.y += fEyeHeight_;

			/* 視線方向(プレイヤーの前方)。*/
			const Vector3 vLook = pPlayer_->GetLookDirection();

			g_camera3D->SetPosition(vEyePos);
			g_camera3D->SetTarget(vEyePos + vLook * 100.0f);
			g_camera3D->Update();
		}


		void DebugShootingRangeScene::SpawnTargetEnemies()
		{
			for (int i = 0; i < iTargetEnemyCount_; ++i)
			{
				aTargetEnemies_[i] = NewGO<nsActor::CommonEnemy>(0, "commonEnemy");
				aTargetEnemies_[i]->SetPosition(aTargetPositions_[i]);
				aTargetEnemies_[i]->SetTarget(pPlayer_);
			}
		}
	}
}