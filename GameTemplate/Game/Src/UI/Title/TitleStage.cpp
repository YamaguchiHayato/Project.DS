#include "stdafx.h"
#include "TitleStage.h"
#include "Src/System/ModelBounds.h"

namespace
{
	const char* sModelPath_ = "Assets/modelData/player/player.tkm";		//! 立たせるモデル。
	const char* sIdleAnimationPath_ = "Assets/animData/player/Idle.tka";	//! 待機アニメーション。
	const int iIdleAnimationNo_ = 0;							//! 待機アニメーションの番号。
	const Vector3 vModelPosition_ = { 0.0f, 0.0f, 0.0f };		//! モデルを置く位置。
	const float fModelTurnDegree_ = 160.0f;						//! モデルの向き(度)。真正面から少し斜めに振る。

	/*
	 * モデルは画面の右側に出す。
	 * モデルを右へ動かすのではなくカメラを左へずらすのは、ライトの当たり方を変えずに寄せられるため。
	 *
	 * 寄り引きは、モデルの実際の表示サイズに対する倍率で決める。
	 * 固定の距離にすると、モデルを差し替えて大きさが変わったとき画面から外れてしまうため。
	 *
	 * 倍率は、モデルが画面の高さの8割を占め、頭が上から3分の1あたりに来る絵から逆算している。
	 * 画角は本編より狭くして、近づいても顔が歪まないようにする。
	 */
	const float fTitleViewAngleDegree_ = 35.0f;	//! タイトルの画角(度)。抜けるときは元へ戻す。
	const float fCameraSideRate_ = -0.60f;	//! カメラを左へずらす量(モデルの大きさに対する倍率)。大きくするほどモデルが右へ寄る。
	const float fCameraBackRate_ = 1.98f;	//! カメラを後ろへ引く距離(同上)。
	const float fCameraUpRate_ = 0.76f;		//! カメラを持ち上げる高さ(同上)。
	const float fCameraLookRate_ = 0.76f;	//! 注視点の高さ(同上。胸から上のあたり)。
	const float fModelDefaultSize_ = 150.0f;	//! モデルを測れなかったときに使う大きさ。

	/*
	 * タイトル用のライト。
	 * 本編は Assets/data/Lighting/overcast.json の夕方の設定で動いていて、そのままだと顔が見えない。
	 * ここで入れた値は TitleStage を抜けるときに元へ戻すので、本編の見た目には影響しない。
	 */
	const Vector3 vKeyLightDirection_ = { 0.3f, -0.4f, 0.86f };		//! 主光源の向き。カメラの後ろ側から当てる。
	const Vector3 vKeyLightColor_ = { 1.1f, 1.0f, 0.95f };			//! 主光源の色。
	const Vector3 vAmbientLightColor_ = { 0.25f, 0.26f, 0.32f };	//! 環境光。影側がつぶれない程度に明るくする。
	const Vector3 vHemiSkyColor_ = { 0.28f, 0.31f, 0.40f };			//! 半球ライトの上の色。
	const Vector3 vHemiGroundColor_ = { 0.10f, 0.09f, 0.08f };		//! 半球ライトの下の色。
	const Vector3 vHemiGroundNormal_ = { 0.0f, 1.0f, 0.0f };		//! 半球ライトの上向き。
}

namespace nsApp
{
	namespace nsUI
	{
		TitleStage::~TitleStage()
		{
			/*
			 * ゲームを終えるときは、グラフィックス側が片付いてから各オブジェクトが壊される。
			 * そのときに共用のカメラやライトへ触ると、すでに無いものを触ることになるので何もしない。
			 * 片付いたかどうかは、K2EngineLow が nullptr にする g_renderingEngine で見分ける。
			 */
			if (g_renderingEngine == nullptr)
				return;

			/* カメラとライトは他のシーンと共用なので、来たときの状態へ戻してから抜ける。*/
			g_camera3D->SetPosition(vSavedCameraPosition_);
			g_camera3D->SetTarget(vSavedCameraTarget_);
			g_camera3D->SetViewAngle(fSavedViewAngle_);
			g_camera3D->Update();
			g_renderingEngine->GetLight() = stSavedLight_;
		}


		bool TitleStage::Start()
		{
			/* 待機アニメーションを読み込み、ずっと繰り返すようにする。*/
			aAnimationClip_[iIdleAnimationNo_].Load(sIdleAnimationPath_);
			aAnimationClip_[iIdleAnimationNo_].SetLoopFlag(true);

			/* モデルを読み込んで、カメラの方を向かせる。*/
			stModelRender_.Init(sModelPath_, aAnimationClip_, iAnimationCount_, enModelUpAxisZ);
			stModelRender_.SetPosition(vModelPosition_);

			Quaternion qRotation;
			qRotation.SetRotationDegY(fModelTurnDegree_);
			stModelRender_.SetRotation(qRotation);
			stModelRender_.PlayAnimation(iIdleAnimationNo_);
			stModelRender_.Update();

			/* 頂点から実際の大きさを測る。測れなければ既定の大きさで置く。*/
			const nsSystem::ModelBounds stBounds = nsSystem::MeasureModelBounds(stModelRender_.GetModel());
			fModelSize_ = stBounds.bIsValid_ ? stBounds.GetLongestEdge() : fModelDefaultSize_;

			/* 書き換える前に、戻すための値を控える。*/
			vSavedCameraPosition_ = g_camera3D->GetPosition();
			vSavedCameraTarget_ = g_camera3D->GetTarget();
			fSavedViewAngle_ = g_camera3D->GetViewAngle();
			stSavedLight_ = g_renderingEngine->GetLight();

			SetUpCamera();
			SetUpLight();
			return true;
		}


		void TitleStage::Update()
		{
			/* 待機アニメーションを進める。*/
			stModelRender_.Update();
		}


		void TitleStage::Render(RenderContext& rc)
		{
			/* メニューの画面になるまでは描かない。*/
			if (!bIsVisible_)
				return;

			/* モデルを描画する。*/
			stModelRender_.Draw(rc);
		}


		void TitleStage::SetUpCamera()
		{
			/* 注視点とカメラを同じだけ横へずらすと、正面から見たままモデルだけが画面の端へ寄る。*/
			const Vector3 vLookAt =
			{
				vModelPosition_.x + fModelSize_ * fCameraSideRate_,
				vModelPosition_.y + fModelSize_ * fCameraLookRate_,
				vModelPosition_.z
			};

			const Vector3 vEyePosition =
			{
				vLookAt.x,
				vModelPosition_.y + fModelSize_ * fCameraUpRate_,
				vModelPosition_.z - fModelSize_ * fCameraBackRate_
			};

			g_camera3D->SetTarget(vLookAt);
			g_camera3D->SetPosition(vEyePosition);
			g_camera3D->SetUp(Vector3::AxisY);
			g_camera3D->SetViewAngle(Math::DegToRad(fTitleViewAngleDegree_));
			g_camera3D->Update();
		}


		void TitleStage::SetUpLight()
		{
			/* 正面から当てて、顔と装備が見えるようにする。*/
			g_renderingEngine->SetDirectionalLight(vKeyLightDirection_, vKeyLightColor_);
			g_renderingEngine->SetAmbientLight(vAmbientLightColor_);
			g_renderingEngine->SetHemLight(vHemiGroundColor_, vHemiSkyColor_, vHemiGroundNormal_);
		}
	}
}
