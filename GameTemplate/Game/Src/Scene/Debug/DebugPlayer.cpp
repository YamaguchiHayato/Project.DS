#include "stdafx.h"
#include "DebugPlayer.h"


namespace nsApp
{
	namespace nsActor
	{
		namespace
		{
			const char* sUnityChanModelPath_ = "Assets/modelData/unityChan.tkm";
			const char* sUnityChanIdlePath_ = "Assets/animData/idle.tka";
			const char* sUnityChanWalkPath_ = "Assets/animData/walk.tka";
			const float fAnimInterpolate_ = 0.2f;
		}


		bool DummyPlayer::Start()
		{
			/* カプセルで壁（PhysicsStaticObject）と当たる。*/
			stCharaCon_.Init(20.0f, 70.0f, vPosition_);

			/* Unityちゃん用Idle/Walkを読み込む。*/
			aAnimationClip_[0].Load(sUnityChanIdlePath_);
			aAnimationClip_[0].SetLoopFlag(true);
			aAnimationClip_[1].Load(sUnityChanWalkPath_);
			aAnimationClip_[1].SetLoopFlag(true);

			/* アニメ付きでモデルを初期化する。*/
			stModelRender_.Init(sUnityChanModelPath_, aAnimationClip_, 2, enModelUpAxisY);
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();
			PlayAnimation(0); // Idle

			/* テスト用のHPを入れる。*/
			stCharacterStatus_.stHp_.iCurrentHP_ = 100;
			stCharacterStatus_.stHp_.iMaxHP_ = 100;

			/* HP表示の位置と大きさを設定する。*/
			stHpFont_.SetPosition(-850.0f, 480.0f, 0.0f);
			stHpFont_.SetScale(1.2f);
			stHpFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			UpdateHpFont();

			return true;
		}

		
		void DummyPlayer::Update()
		{
			ICharacter::Update();
			Move();

			/* 移動状態でIdle/Walk切替。*/
			PlayAnimation(bIsMoving_ ? 1 : 0);

			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();
			UpdateHpFont();
		}


		void DummyPlayer::Move()
		{
			const float fStickX = g_pad[0]->GetLStickXF();
			const float fStickY = g_pad[0]->GetLStickYF();
			Vector3 vDir = { fStickX, 0.0f, fStickY };

			Vector3 vSpeed = Vector3::Zero;
			if (vDir.Length() < 0.1f)
			{
				bIsMoving_ = false;
			}
			else
			{
				bIsMoving_ = true;
				vDir.Normalize();
				vSpeed = vDir * fMoveSpeed_;

				Quaternion qRot;
				qRot.SetRotationY(atan2f(vDir.x, vDir.z));
				stModelRender_.SetRotation(qRot);
			}

			/* 座標直書きせず、キャラコンで衝突解決した位置を使う。*/
			vPosition_ = stCharaCon_.Execute(vSpeed, g_gameTime->GetFrameDeltaTime());
		}


		void DummyPlayer::PlayAnimation(int iAnimationNumber)
		{
			if (iPlayingAnimation_ == iAnimationNumber)
				return;

			iPlayingAnimation_ = iAnimationNumber;
			stModelRender_.PlayAnimation(iAnimationNumber, fAnimInterpolate_);
		}


		void DummyPlayer::Render(RenderContext& rc)
		{
			/* 仮モデルを描画する。*/
			stModelRender_.Draw(rc);

			/* HPを描画する。*/
			stHpFont_.Draw(rc);
		}


		void DummyPlayer::UpdateHpFont()
		{
			/* 現在HPと最大HPを文字列にする。*/
			swprintf_s(aHpText_, L"HP %d / %d", GetCurrentHP(), stCharacterStatus_.stHp_.iMaxHP_);
			stHpFont_.SetText(aHpText_);
		}
	}
}