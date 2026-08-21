#include "stdafx.h"
#include "InGameScene.h"
#include "Player.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Scene/GameFlow.h"

namespace
{
	const float	fEyeHeight_ = 160.0f;								//! 目(カメラ)の高さ。Player側の kEyeHeight と合わせる。
	const char* sGroundModelPath_ = "Assets/modelData/ground.tkm";	//! 地面モデル。
	const float	fGroundScale_ = 200.0f;								//! 地面の拡大率。
	const char* sGoalBeaconModelPath_ = "Assets/modelData/preset/VolumePointLight.tkm";	//! ゴール目印(発光球)。
	const float	fSafeRoomMarkerScale_ = 80.0f;						//! ゴール目印の拡大率(遠くからでも見える大きさ)。
	const float	fSafeRoomMarkerHeight_ = 120.0f;					//! ゴール目印を目線あたりに浮かせる高さ。
}

namespace nsApp
{
	namespace nsScene
	{
		InGameScene::~InGameScene()
		{
			/*
			 * 残っている敵を全て破棄する。ヒットスキャンで先に倒された個体は
			 * FindGOs に含まれないため、二重破棄(ダングリング)にならない。
			 */
			for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
				DeleteGO(pEnemy);
			pTestEnemy_ = nullptr;

			/* 生成したプレイヤーを破棄する。*/
			if (pPlayer_ != nullptr)
			{
				DeleteGO(pPlayer_);
				pPlayer_ = nullptr;
			}
		}


		bool InGameScene::Start()
		{
			/* カメラを初期化する。*/
			InitCamera();

			/* 地面を読み込んで大きく敷く。*/
			stGroundModel_.Init(sGroundModelPath_, nullptr, 0, enModelUpAxisY);
			stGroundModel_.SetScale(Vector3(fGroundScale_, fGroundScale_, fGroundScale_));
			stGroundModel_.SetPosition(Vector3(0.0f, 0.0f, 0.0f));
			stGroundModel_.Update();

			/* ゴール(セーフルーム)の目印を置く。ここへ到達で勝利。発光球を目線高さに浮かせて視認性を確保する。*/
			stSafeRoomModel_.Init(sGoalBeaconModelPath_, nullptr, 0, enModelUpAxisY);
			stSafeRoomModel_.SetScale(Vector3(fSafeRoomMarkerScale_, fSafeRoomMarkerScale_, fSafeRoomMarkerScale_));
			{
				Vector3 vMarkerPos = vSafeRoomPos_;
				vMarkerPos.y += fSafeRoomMarkerHeight_;
				stSafeRoomModel_.SetPosition(vMarkerPos);
			}
			stSafeRoomModel_.Update();

			/* プレイヤーを生成する。*/
			pPlayer_ = NewGO<nsActor::Player>(0, "player");

			/*
			 * ★v1動作確認用: 雑魚敵を1体だけ生成し、プレイヤーを標的にする。
			 *   敵はこれだけで追跡・攻撃(プレイヤーへの ApplyDamage)まで動く。
			 *   EnemyDirector(波湧き)を実装したら、この直接生成は取り除く。
			 */
			pTestEnemy_ = NewGO<nsActor::CommonEnemy>(0, "commonEnemy");
			pTestEnemy_->SetTarget(pPlayer_);

			/* 生成直後のプレイヤー位置へカメラを合わせる。*/
			UpdateCamera();
			return true;
		}


		void InGameScene::Update()
		{
			/* 一人称カメラをプレイヤーへ追従させる。*/
			UpdateCamera();

			/* 勝敗判定(到達=勝ち/死亡=負け)。*/
			UpdateResultJudge();
		}


		void InGameScene::Render(RenderContext& rc)
		{
			/* 地面を描画する。*/
			stGroundModel_.Draw(rc);

			/* ゴールの目印を描画する。*/
			stSafeRoomModel_.Draw(rc);
		}


		void InGameScene::InitCamera()
		{
			/* ニアクリップ・ファークリップを設定する。*/
			g_camera3D->SetNear(1.0f);
			g_camera3D->SetFar(10000.0f);
		}


		void InGameScene::UpdateCamera()
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


		void InGameScene::UpdateResultJudge()
		{
			/* 既に予約済み、または前提が無ければ何もしない。*/
			if (bResultRequested_ || pPlayer_ == nullptr || pGameFlow_ == nullptr)
				return;

			bool bDecided = false;
			bool bWon = false;

			/* 勝利: セーフルームへ到達したか(高さ無視の水平距離)。*/
			Vector3 vToGoal = vSafeRoomPos_ - pPlayer_->GetPosition();
			vToGoal.y = 0.0f;
			if (vToGoal.Length() <= fSafeRoomRadius_)
			{
				bDecided = true;
				bWon = true;
			}
			/* 敗北: プレイヤーが死亡したか。*/
			else if (pPlayer_->IsDead())
			{
				bDecided = true;
				bWon = false;
			}

			/* 決着したら結果を記録してリザルトへ遷移予約する。*/
			if (bDecided)
			{
				pGameFlow_->SetMatchWon(bWon);
				pGameFlow_->ChangeScene(EnSceneID::Result);
				bResultRequested_ = true;
			}
		}
	}
}
