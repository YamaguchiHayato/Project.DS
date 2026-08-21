#include "stdafx.h"
#include "DebugPlayer.h"

namespace
{
	const char* sUnityChanModelPath_ = "Assets/modelData/unityChan.tkm"; //! Unityちゃんのモデルパス。
}

namespace nsApp
{
	namespace nsActor
	{
		bool DummyPlayer::Start()
		{
			/* 仮モデルをロードする。*/
			stModelRender_.Init(sUnityChanModelPath_, nullptr, 0, enModelUpAxisZ);
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();

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
			/* 基底の更新。*/
			ICharacter::Update();

			/* パッドで移動する。*/
			Move();

			/* モデルを更新する。*/
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();

			/* HP表示を更新する。*/
			UpdateHpFont();
		}


		void DummyPlayer::Render(RenderContext& rc)
		{
			/* 仮モデルを描画する。*/
			stModelRender_.Draw(rc);

			/* HPを描画する。*/
			stHpFont_.Draw(rc);
		}


		void DummyPlayer::Move()
		{
			/* 左スティックの入力を取る。*/
			const float fStickX = g_pad[0]->GetLStickXF();
			const float fStickY = g_pad[0]->GetLStickYF();
			Vector3 vMove = { fStickX, 0.0f, fStickY };

			/* 入力が無ければ移動しない。*/
			if (vMove.Length() < 0.1f)
				return;

			/* 入力方向へ移動する。*/
			vMove.Normalize();
			vPosition_ += vMove * fMoveSpeed_ * g_gameTime->GetFrameDeltaTime();
		}


		void DummyPlayer::UpdateHpFont()
		{
			/* 現在HPと最大HPを文字列にする。*/
			swprintf_s(aHpText_, L"HP %d / %d", GetCurrentHP(), stCharacterStatus_.stHp_.iMaxHP_);
			stHpFont_.SetText(aHpText_);
		}
	}
}