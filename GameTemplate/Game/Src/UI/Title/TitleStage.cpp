#include "stdafx.h"
#include "TitleStage.h"

namespace
{
	/*
	 * 立たせるモデルとモーション。
	 * @todo 使う PMX が決まったらパスを差し替える。モーションは無くても立ち姿で出る。
	 */
	const char* sModelPath_ = "Assets/modelData/title/player.pmx";	//! 立たせるモデル。
	const char* sMotionPath_ = "Assets/animData/title/Idle.vmd";	//! 待機モーション。

	const DirectX::XMFLOAT3 vModelPosition_ = { 0.0f, 0.0f, 0.0f };	//! モデルを置く位置。
	const float fModelTurnDegree_ = 20.0f;	//! モデルの向き(度)。PMXは最初からカメラの方を向くので、少しだけ斜めに振る。
	const float fMotionFramePerSecond_ = 30.0f;	//! VMDのフレームの基準。秒へ直すのに使う。
	const float fMotionLoopBackSecond_ = 0.5f;	//! モーションの時刻がこれ以上戻ったら、繰り返しの折り返しとみなす。

	/*
	 * モデルは画面の右側に出す。
	 * モデルを右へ動かすのではなくカメラを左へずらすのは、光の当たり方を変えずに寄せられるため。
	 *
	 * 寄り引きは、モデルの実際の背丈に対する倍率で決める。
	 * 固定の距離にすると、モデルを差し替えて大きさが変わったとき画面から外れてしまうため。
	 * PMXはMMDの単位(背丈20ほど)で、tkmとは桁が違う。倍率で持つことで、どちらでも同じ絵になる。
	 *
	 * 倍率は、モデルが画面の高さの8割を占め、頭が上から3分の1あたりに来る絵から逆算している。
	 * 画角は本編より狭くして、近づいても顔が歪まないようにする。
	 */
	const float fTitleViewAngleDegree_ = 35.0f;	//! メニュー画面の画角(度)。抜けるときは元へ戻す。
	const float fCameraSideRate_ = -0.60f;	//! カメラを左へずらす量(モデルの背丈に対する倍率)。大きくするほどモデルが右へ寄る。
	const float fCameraBackRate_ = 1.98f;	//! カメラを後ろへ引く距離(同上)。
	const float fCameraUpRate_ = 0.76f;		//! カメラを持ち上げる高さ(同上)。
	const float fCameraLookRate_ = 0.76f;	//! 注視点の高さ(同上。胸から上のあたり)。
	const float fModelDefaultHeight_ = 20.0f;	//! モデルを測れなかったときに使う背丈(MMDの単位)。

	const float fCameraNear_ = 0.1f;	//! カメラの手前の面。MMDの単位に合わせて本編より近くする。
	const float fCameraFar_ = 1000.0f;	//! カメラの奥の面。

	/**
	 * @brief 頂点を走査して、モデルの背丈を測る。
	 * @param stModel 測るモデル。
	 * @return 背丈。頂点が取れなければ0。
	 * @details PMXはY軸が上の立ち姿なので、Yの幅をそのまま背丈として使える。
	 *          一番長い辺にしないのは、両手を広げた姿勢だと横幅のほうが長くなるため。
	 */
	float MeasureModelHeight(const PMXModel& stModel)
	{
		if (stModel.vertices.empty())
			return 0.0f;

		float fMinY = stModel.vertices[0].pos[1];
		float fMaxY = fMinY;

		for (const PMXVertex& stVertex : stModel.vertices)
		{
			const float fPositionY = stVertex.pos[1];
			if (fPositionY < fMinY)
				fMinY = fPositionY;

			if (fPositionY > fMaxY)
				fMaxY = fPositionY;
		}

		return fMaxY - fMinY;
	}

	/**
	 * @brief Y軸まわりの回転を作る。
	 * @param fDegree 回す角度(度)。
	 * @return 回転。
	 */
	DirectX::XMFLOAT4 MakeTurnY(float fDegree)
	{
		/* 四元数はsin・cosに半分の角度を渡す。*/
		const float fHalfRadian = Math::DegToRad(fDegree) * 0.5f;
		return DirectX::XMFLOAT4(0.0f, sinf(fHalfRadian), 0.0f, cosf(fHalfRadian));
	}
}

namespace nsApp
{
	namespace nsUI
	{
		TitleStage::~TitleStage()
		{
			/*
			 * ゲームを終えるときは、グラフィックス側が片付いてから各オブジェクトが壊される。
			 * そのときに共用のカメラや描画エンジンへ触ると、すでに無いものを触ることになるので何もしない。
			 * 片付いたかどうかは、K2EngineLow が nullptr にする g_renderingEngine で見分ける。
			 */
			if (g_renderingEngine == nullptr)
				return;

			/* 描画エンジンに残さない。*/
			RegisterModel(false);

			/* カメラは他のシーンと共用なので、来たときの状態へ戻してから抜ける。*/
			g_camera3D->SetPosition(vSavedCameraPosition_);
			g_camera3D->SetTarget(vSavedCameraTarget_);
			g_camera3D->SetViewAngle(fSavedViewAngle_);
			g_camera3D->SetNear(fSavedNear_);
			g_camera3D->SetFar(fSavedFar_);
			g_camera3D->Update();
		}


		bool TitleStage::Start()
		{
			/* 書き換える前に、戻すための値を控える。*/
			vSavedCameraPosition_ = g_camera3D->GetPosition();
			vSavedCameraTarget_ = g_camera3D->GetTarget();
			fSavedViewAngle_ = g_camera3D->GetViewAngle();
			fSavedNear_ = g_camera3D->GetNear();
			fSavedFar_ = g_camera3D->GetFar();

			/* 読み込めなくても画面は出す。モデルだけが出ない形にする。*/
			bIsLoaded_ = LoadModel();
			if (!bIsLoaded_)
				return true;

			SetUpCamera();
			stMotionClock_.Start();
			return true;
		}


		void TitleStage::Update()
		{
			/* 読み込めていなければ何もしない。*/
			if (!bIsLoaded_)
				return;

			/* 見せるかどうかに合わせて、描画エンジンへの登録を切り替える。*/
			RegisterModel(bIsVisible_);

			/* 隠している間はモーションも進めない。*/
			if (!bIsVisible_)
				return;

			UpdateMotion();
		}


		bool TitleStage::LoadModel()
		{
			/* PMXを読み込む。頂点が1つも無ければ読めていない。*/
			stPmxLoader_.LoadFromFile(sModelPath_, stPmxModel_);
			if (stPmxModel_.vertices.empty())
			{
				DebugPrintW(L"[TitleStage] PMXを読み込めなかった。パスを確認すること。\n");
				return false;
			}

			/* 描き込む先に合わせて初期化し、テクスチャを読み終えるまで進める。*/
			RenderTarget& stMainRenderTarget = g_renderingEngine->GetMainRenderTarget();
			if (!stPmxRender_.Init(stPmxModel_, stMainRenderTarget.GetColorBufferFormat(), stMainRenderTarget.GetDepthBufferFormat()))
			{
				DebugPrintW(L"[TitleStage] PMXの描画の初期化に失敗した。\n");
				return false;
			}

			while (stPmxRender_.UpdateLoader()) {}

			/* 待機モーションを読み込む。無くても立ち姿で出せるので、読めなくても続ける。*/
			if (stVmdLoader_.Load(sMotionPath_))
			{
				stPmxRender_.AttachVMD(&stVmdLoader_);

				/*
				 * モーションの終わりを秒へ直して、繰り返しの周期にする。
				 * SyncToMusicTime は終わりで止まる作りなので、折り返すのはこちらの仕事。
				 */
				const float fEndFrame = stPmxRender_.GetVMDEndFrameDebug();
				fMotionEndSecond_ = (fEndFrame > 0.0f) ? (fEndFrame / fMotionFramePerSecond_) : 0.0f;
			}

			/* モーションにカメラを乗っ取らせない。こちらで置いたカメラが動いてしまうため。*/
			stPmxRender_.SetEnableCameraVMD(false);
			stPmxRender_.SetAnimPlaySpeed(1.0f);
			stPmxRender_.SetRootTransform(vModelPosition_, MakeTurnY(fModelTurnDegree_));
			stPmxRender_.ResetPhysics();

			/* カメラの寄り引きに使うので、背丈を測っておく。*/
			const float fHeight = MeasureModelHeight(stPmxModel_);
			fModelHeight_ = (fHeight > 0.0f) ? fHeight : fModelDefaultHeight_;
			return true;
		}


		void TitleStage::RegisterModel(bool bIsRegister)
		{
			/* すでにその状態なら何もしない。二重に登録したり解除したりしないため。*/
			if (bIsRegister == bIsRegistered_)
				return;

			if (bIsRegister)
				g_renderingEngine->AddPMXObject(&stPmxRender_);
			else
				g_renderingEngine->RemovePMXObject(&stPmxRender_);

			bIsRegistered_ = bIsRegister;
		}


		void TitleStage::UpdateMotion()
		{
			/* 立たせてからの実経過秒を取り、繰り返しの範囲へ畳む。*/
			stMotionClock_.Stop();
			const float fElapsed = static_cast<float>(stMotionClock_.GetElapsed());
			const float fMotionSecond = (fMotionEndSecond_ > 0.0f) ? fmodf(fElapsed, fMotionEndSecond_) : fElapsed;

			/*
			 * 終わりから先頭へ一息で飛ぶと、髪やスカートの物理が引きずられて暴れる。
			 * 折り返しを見つけたら物理を入れ直して、新しい姿勢で落ち着かせ直す。
			 */
			if (fMotionSecond < fPrevMotionSecond_ - fMotionLoopBackSecond_)
				stPmxRender_.ResetPhysics();

			fPrevMotionSecond_ = fMotionSecond;
			stPmxRender_.SyncToMusicTime(fMotionSecond);
		}


		void TitleStage::SetUpCamera()
		{
			/* 注視点とカメラを同じだけ横へずらすと、正面から見たままモデルだけが画面の端へ寄る。*/
			const Vector3 vLookAt =
			{
				vModelPosition_.x + fModelHeight_ * fCameraSideRate_,
				vModelPosition_.y + fModelHeight_ * fCameraLookRate_,
				vModelPosition_.z
			};

			const Vector3 vEyePosition =
			{
				vLookAt.x,
				vModelPosition_.y + fModelHeight_ * fCameraUpRate_,
				vModelPosition_.z - fModelHeight_ * fCameraBackRate_
			};

			g_camera3D->SetTarget(vLookAt);
			g_camera3D->SetPosition(vEyePosition);
			g_camera3D->SetUp(Vector3::AxisY);
			g_camera3D->SetViewAngle(Math::DegToRad(fTitleViewAngleDegree_));

			/* MMDの単位は本編より桁が小さいので、手前と奥の面も合わせて詰める。*/
			g_camera3D->SetNear(fCameraNear_);
			g_camera3D->SetFar(fCameraFar_);
			g_camera3D->Update();
		}
	}
}
