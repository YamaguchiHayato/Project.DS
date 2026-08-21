#include "stdafx.h"
#include "ResolveGameWinner.h"
#include "Src/Event/EventBus.h"

namespace nsApp
{
	namespace nsRule
	{
		bool ResolveGameWinner::Start()
		{
			/* イベントバスを見つけて購読する。*/
			pEventBus_ = FindGO<nsEvent::EventBus>("eventBus");
			if (pEventBus_ != nullptr)
				pEventBus_->Subscribe(this);
			return true;
		}


		ResolveGameWinner::~ResolveGameWinner()
		{
			/*
			 * 購読解除は「所有者(InGameScene)が破棄前に行う」方針。
			 * ここで pEventBus_ を触ると、遅延破棄時に解放後のバスへ触れる恐れがあるため何もしない。
			 */
		}


		void ResolveGameWinner::OnGameEvent(const nsEvent::GameEvent& stEvent)
		{
			/* 既に決着していれば上書きしない(最初の決着を採用)。*/
			if (enPhase_ != EnGamePhase::Playing)
				return;

			/* イベント種別に応じて勝敗を確定する。*/
			switch (stEvent.enType_)
			{
			case nsEvent::EnGameEvent::PlayerReachedSafeRoom:
				/* セーフルーム到達で勝利。*/
				enPhase_ = EnGamePhase::Won;
				break;

			case nsEvent::EnGameEvent::PlayerDead:
				/* プレイヤー死亡で敗北。*/
				enPhase_ = EnGamePhase::Lost;
				break;

			default:
				/* それ以外は現状の勝敗に影響しない。*/
				break;
			}
		}
	}
}
