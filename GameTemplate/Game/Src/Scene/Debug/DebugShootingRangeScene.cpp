#include "stdafx.h"
#include "DebugShootingRangeScene.h"
#include "Src/Scene/GameFlow.h"
#include "Player.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"

namespace
{
	const int iTargetEnemyCount_ = 12;

	/**
	 * @brief 視線を軸にして「上」を回し、傾けたカメラの上方向を作る。
	 * @param vLook 視線方向(正規化済み)。
	 * @param fRoll 傾ける角度(ラジアン)。
	 * @return 傾けた上方向。
	 */
	Vector3 MakeCameraUp(const Vector3& vLook, float fRoll)
	{
		Vector3 vUp = Vector3::AxisY;

		/* 傾きが無ければ真上のまま返す。*/
		if (fRoll == 0.0f)
			return vUp;

		Quaternion qRoll;
		qRoll.SetRotation(vLook, fRoll);
		qRoll.Apply(vUp);

		return vUp;
	}
	const char* sStageModelPath_ = "Assets/modelData/stage/DebugStage/demoStage.tkm";
	const Vector3 vHintFontPos_ = { -450.0f, 450.0f, 0.0f };
	const int iViewModeKey_ = 'T';				//! 一人称/三人称を切り替えるキー。
	/*
	 * 三人称カメラの寄り引きは、モデルの実際の表示サイズに対する倍率で決める。
	 * 固定の距離にすると、モデルを差し替えて大きさが変わったとき画面から外れてしまうため。
	 */
	const float fThirdPersonBackRate_ = 2.2f;		//! カメラを後ろへ引く距離(モデルの大きさに対する倍率)。
	const float fThirdPersonUpRate_ = 0.5f;			//! カメラを持ち上げる高さ(同上)。
	const float fThirdPersonLookRate_ = 0.55f;		//! 注視点の高さ(同上。胸のあたり)。
	const float fThirdPersonMinSize_ = 50.0f;		//! モデルを測れなかったときに使う大きさ。
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
			/* カメラの傾きを戻す(g_camera3Dは他のシーンと共用なので傾いたままにしない)。*/
			g_camera3D->SetUp(Vector3::AxisY);

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
			stHintFont_.SetText(L"SHOOTING RANGE  ESC BACK  T VIEW(1st/3rd)");

			/* 前フレームのESC状態を取る。 */
			bWasPressEsc_ = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
			bWasPressViewKey_ = (GetAsyncKeyState(iViewModeKey_) & 0x8000) != 0;
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

			/* カメラをプレイヤーへ追従させる。*/
			UpdateCamera();

			/* デバッグ用の処理はまとめてここで回す。*/
			UpdateViewModeSwitch();
			UpdateDebugHint();
		}


		void DebugShootingRangeScene::UpdateViewModeSwitch()
		{
			/* プレイヤーが無ければ切り替えられない。*/
			if (pPlayer_ == nullptr)
				return;

			const bool bPressViewKey = (GetAsyncKeyState(iViewModeKey_) & 0x8000) != 0;

			/* 押した瞬間だけ切り替える。*/
			if (bPressViewKey && !bWasPressViewKey_)
			{
				bIsThirdPersonView_ = !bIsThirdPersonView_;

				/* 三人称にすると体が描かれ、銃が右手のボーンへ移る。*/
				pPlayer_->SetViewMode(bIsThirdPersonView_
					? nsActor::EnViewMode::ThirdPerson
					: nsActor::EnViewMode::FirstPerson);
			}

			/* 次フレーム判定用に今の状態を残す。*/
			bWasPressViewKey_ = bPressViewKey;
		}


		void DebugShootingRangeScene::UpdateDebugHint()
		{
			/* プレイヤーが無ければ出す値が無い。*/
			if (pPlayer_ == nullptr)
				return;

			/*
			 * いまの視点と、体モデルの実測サイズを画面に出す。
			 * キーが効いているかと、モデルの大きさが想定(目線の高さ)と合っているかをここで確かめる。
			 */
			const Vector3& vGunPos = pPlayer_->GetWeaponViewPosition();
			const Vector3& vPlayerPos = pPlayer_->GetPosition();

			swprintf_s(wcHint_, L"T VIEW=%s  size=%.1f eye=%.1f  hand=%s  gun(%.0f,%.0f,%.0f) ply(%.0f,%.0f,%.0f)",
				bIsThirdPersonView_ ? L"3rd" : L"1st",
				pPlayer_->GetBodyModelSize(),
				pPlayer_->GetEyePosition().y - vPlayerPos.y,
				pPlayer_->IsHandBoneFound() ? L"OK" : L"NG",
				vGunPos.x, vGunPos.y, vGunPos.z,
				vPlayerPos.x, vPlayerPos.y, vPlayerPos.z);

			stHintFont_.SetText(wcHint_);
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

			/* 目の位置(歩きの上下動を含む)。射撃の起点と同じ値をプレイヤーから受け取る。*/
			const Vector3 vEyePos = pPlayer_->GetEyePosition();

			/* 視線方向(プレイヤーの前方)。*/
			const Vector3 vLook = pPlayer_->GetLookDirection();

			if (bIsThirdPersonView_)
			{
				/*
				 * 他人から見た姿を確認するため、後ろへ引いて胸のあたりを見る。
				 * 距離はモデルの実際の大きさから決めるので、モデルを差し替えても画面に収まる。
				 */
				float fBodySize = pPlayer_->GetBodyModelSize();
				if (fBodySize <= 0.0f)
					fBodySize = fThirdPersonMinSize_;

				Vector3 vLookPos = pPlayer_->GetPosition();
				vLookPos.y += fBodySize * fThirdPersonLookRate_;

				/* 上下を向いてもカメラが地面へ潜らないよう、引く向きは水平だけにする。*/
				Vector3 vBack = { vLook.x, 0.0f, vLook.z };
				vBack.Normalize();

				Vector3 vCameraPos = vLookPos - vBack * (fBodySize * fThirdPersonBackRate_);
				vCameraPos.y += fBodySize * fThirdPersonUpRate_;

				g_camera3D->SetPosition(vCameraPos);
				g_camera3D->SetTarget(vLookPos);
			}
			else
			{
				g_camera3D->SetPosition(vEyePos);
				g_camera3D->SetTarget(vEyePos + vLook * 100.0f);
			}

			/* 歩きと横移動に合わせてカメラをわずかに傾ける。*/
			g_camera3D->SetUp(MakeCameraUp(vLook, pPlayer_->GetViewRoll()));
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