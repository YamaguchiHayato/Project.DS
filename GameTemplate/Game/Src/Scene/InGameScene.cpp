#include "stdafx.h"
#include "InGameScene.h"
#include "Player.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Scene/GameFlow.h"
#include "Src/Event/EventBus.h"
#include "Src/Rule/ResolveGameWinner.h"
#include "Src/UI/InGameHud.h"
#include "Src/Director/EnemyDirector.h"
#include "Src/System/GamePause.h"

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
			/* エフェクトを片付け、リストを無効化する。*/
			nsEffect::EffectList::SetActiveList(nullptr);
			stEffectList_.Clear();

			/* 敵の湧き係を破棄する(以降の生成を止める)。*/
			if (pEnemyDirector_ != nullptr)
			{
				DeleteGO(pEnemyDirector_);
				pEnemyDirector_ = nullptr;
			}

			/*
			 * 残っている敵を全て破棄する。ヒットスキャンで先に倒された個体は
			 * FindGOs に含まれないため、二重破棄(ダングリング)にならない。
			 */
			for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
				DeleteGO(pEnemy);

			/* 生成したプレイヤーを破棄する。*/
			if (pPlayer_ != nullptr)
			{
				DeleteGO(pPlayer_);
				pPlayer_ = nullptr;
			}

			/* HUDを破棄する。*/
			if (pHud_ != nullptr)
			{
				DeleteGO(pHud_);
				pHud_ = nullptr;
			}

			/* エフェクト再生係の購読を解除する。*/
			if (pEventBus_ != nullptr)
				pEventBus_->Unsubscribe(&stEffectListener_);

			/* ルールをバスから購読解除してから、ルール・バスを破棄する(解放後アクセス防止)。*/
			if (pEventBus_ != nullptr && pGameRule_ != nullptr)
				pEventBus_->Unsubscribe(pGameRule_);
			if (pGameRule_ != nullptr)
			{
				DeleteGO(pGameRule_);
				pGameRule_ = nullptr;
			}
			if (pEventBus_ != nullptr)
			{
				DeleteGO(pEventBus_);
				pEventBus_ = nullptr;
			}
		}


		bool InGameScene::Start()
		{
			/* シーン開始時はポーズを解除しておく(前回のポーズが残らないように)。*/
			nsSystem::SetGamePaused(false);

			/* エフェクト素材を登録し、このシーンのリストを有効にする(未配置の素材は無視される)。*/
			stEffectList_.Init();
			nsEffect::EffectList::SetActiveList(&stEffectList_);

			/* カメラを初期化する。*/
			InitCamera();

			/* イベントバスと勝敗管理を先に生成する(他GOが購読/発行できるように)。*/
			pEventBus_ = NewGO<nsEvent::EventBus>(0, "eventBus");
			pGameRule_ = NewGO<nsRule::ResolveGameWinner>(0, "gameRule");

			/* エフェクト再生係に通知を購読させる(発行元は演出を知らなくてよい)。*/
			stEffectListener_.Initialize(&stEffectList_);
			pEventBus_->Subscribe(&stEffectListener_);

			/* 地面を読み込んで大きく敷く。*/
			stGroundModel_.Init(sGroundModelPath_, nullptr, 0, enModelUpAxisY);
			stGroundModel_.SetScale(Vector3(fGroundScale_, fGroundScale_, fGroundScale_));
			stGroundModel_.SetPosition(Vector3(0.0f, 0.0f, 0.0f));
			stGroundModel_.Update();

			/* 地面の静的コライダを作る。これが無いとキャラクターが接地できない。*/
			stGroundCollider_.CreateFromModel(
				stGroundModel_.GetModel(),
				stGroundModel_.GetModel().GetWorldMatrix());

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

			/* 敵の湧き係(EnemyDirector)を生成する。時間・同時数上限に応じて雑魚敵を湧かせ続ける。*/
			pEnemyDirector_ = NewGO<nsDirector::EnemyDirector>(0, "enemyDirector");

			/* HUDを生成する(毎フレーム player を参照して数値を表示)。*/
			pHud_ = NewGO<nsUI::InGameHud>(0, "inGameHud");

			/* 生成直後のプレイヤー位置へカメラを合わせる。*/
			UpdateCamera();
			return true;
		}


		void InGameScene::Update()
		{
			/* ポーズメニューの入力: Enterでタイトルへ戻る(エッジ検出)。*/
			const bool bEnter = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
			const bool bEnterTrigger = bEnter && !bPrevEnter_;
			bPrevEnter_ = bEnter;

			if (nsSystem::IsGamePaused())
			{
				if (bEnterTrigger && pGameFlow_ != nullptr)
				{
					nsSystem::SetGamePaused(false);
					pGameFlow_->ChangeScene(EnSceneID::Title);
				}
				return;
			}

			/* 再生中エフェクトの寿命を進める。*/
			stEffectList_.Update(g_gameTime->GetFrameDeltaTime());

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
			const Vector3& vPlayerPos = pPlayer_->GetPosition();

			/* 目の位置(プレイヤー座標＋目線の高さ)。*/
			Vector3 vEyePos = vPlayerPos;
			vEyePos.y += fEyeHeight_;

			/* 視線方向(旋回角の水平前方)。*/
			const Vector3 vLook = pPlayer_->GetLookDirection();

			g_camera3D->SetPosition(vEyePos);
			g_camera3D->SetTarget(vEyePos + vLook * 100.0f);
			g_camera3D->Update();
		}


		void InGameScene::UpdateResultJudge()
		{
			/* プレイヤーが無ければ判定できない。*/
			if (pPlayer_ == nullptr)
				return;

			/* --- センサ: 状況をイベントとして発行する(各種一度だけ) --- */
			if (pEventBus_ != nullptr)
			{
				/* セーフルーム到達(高さ無視の水平距離)。*/
				if (!bReachPublished_)
				{
					Vector3 vToGoal = vSafeRoomPos_ - pPlayer_->GetPosition();
					vToGoal.y = 0.0f;
					if (vToGoal.Length() <= fSafeRoomRadius_)
					{
						pEventBus_->Publish({ nsEvent::EnGameEvent::PlayerReachedSafeRoom });
						bReachPublished_ = true;
					}
				}

				/*
				 * プレイヤーの死亡宣言は Player 側が生命状態(ダウン→出血→死亡)に基づいて
				 * EventBus へ発行する。ここでは検知しない(HP0=即敗北にしないため)。
				 */
			}

			/* --- 橋渡し: 勝敗管理が決着していればリザルトへ遷移予約する --- */
			if (!bResultRequested_ && pGameRule_ != nullptr && pGameRule_->IsOver() && pGameFlow_ != nullptr)
			{
				pGameFlow_->SetMatchWon(pGameRule_->IsWin());
				pGameFlow_->ChangeScene(EnSceneID::Result);
				bResultRequested_ = true;
			}
		}
	}
}
