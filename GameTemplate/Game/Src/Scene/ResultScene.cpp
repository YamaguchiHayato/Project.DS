#include "stdafx.h"
#include "ResultScene.h"
#include "Src/Scene/GameFlow.h"

namespace
{
	const Vector3	vResultFontPos_ = { -260.0f, 80.0f, 0.0f };		//! 見出しの表示位置(画面中央基準)。
	const Vector3	vGuideFontPos_ = { -300.0f, -80.0f, 0.0f };		//! ガイドの表示位置。
	const float		fResultFontScale_ = 2.0f;						//! 見出しの大きさ。
	const float		fGuideFontScale_ = 1.0f;						//! ガイドの大きさ。
	const float		fInputGuardTime_ = 0.4f;						//! 入力を受け付けるまでの猶予(秒)。
	const Vector3 vRecordPosition_ = { -230.0f, 0.0f, 0.0f };	//! 戦績の表示位置。
	const float fRecordFontScale_ = 1.0f;					//! 戦績の文字の大きさ。
}

namespace nsApp
{
	namespace nsScene
	{
		bool ResultScene::Start()
		{
			/* GameFlow が保持する勝敗を取り出す(無ければ敗北扱い)。*/
			const bool bWon = (pGameFlow_ != nullptr) ? pGameFlow_->IsMatchWon() : false;

			/* 勝敗の見出しを作る。*/
			stResultFont_.SetPosition(vResultFontPos_);
			stResultFont_.SetScale(fResultFontScale_);
			if (bWon)
			{
				/* 勝利表示(緑)。*/
				stResultFont_.SetColor(0.4f, 1.0f, 0.4f, 1.0f);
				stResultFont_.SetText(L"MISSION CLEAR");
			}
			else
			{
				/* 敗北表示(赤)。*/
				stResultFont_.SetColor(1.0f, 0.3f, 0.3f, 1.0f);
				stResultFont_.SetText(L"YOU DIED");
			}

			/* 操作ガイド。*/
			stGuideFont_.SetPosition(vGuideFontPos_);
			stGuideFont_.SetScale(fGuideFontScale_);
			stGuideFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stGuideFont_.SetText(L"PRESS ENTER TO RETURN TO TITLE");

			/* 戦績(撃破数とかかった時間)を組み立てて表示する。*/
			if (pGameFlow_ != nullptr)
			{
				const float fTime = pGameFlow_->GetMatchClearTime();
				const int iMinutes = static_cast<int>(fTime) / 60;
				const int iSeconds = static_cast<int>(fTime) % 60;
				swprintf_s(wcRecord_, L"KILLS %d      TIME %d:%02d", pGameFlow_->GetMatchKillCount(), iMinutes, iSeconds);
			}

			stRecordFont_.SetPosition(vRecordPosition_);
			stRecordFont_.SetScale(fRecordFontScale_);
			stRecordFont_.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			stRecordFont_.SetText(wcRecord_);
			return true;
		}


		void ResultScene::Update()
		{
			/* 直前の入力での即抜けを防ぐため、少しの間は受け付けない。*/
			fInputGuardTimer_ += g_gameTime->GetFrameDeltaTime();
			if (fInputGuardTimer_ < fInputGuardTime_)
				return;

			/* Enter でタイトルへ戻る。*/
			if ((GetAsyncKeyState(VK_RETURN) & 0x8000) != 0 && pGameFlow_ != nullptr)
			{
				pGameFlow_->ChangeScene(EnSceneID::Title);
			}
		}


		void ResultScene::Render(RenderContext& rc)
		{
			/* 見出しとガイドを描画する。*/
			stResultFont_.Draw(rc);
			stRecordFont_.Draw(rc);
			stGuideFont_.Draw(rc);
		}
	}
}
