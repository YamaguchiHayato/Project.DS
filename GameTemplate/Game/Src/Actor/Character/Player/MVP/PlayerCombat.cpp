#include "stdafx.h"
#include "Src/Actor/Character/Player/MVP/PlayerCombat.h"
#include "Src/Actor/Character/Player/PlayerIntent.h"
#include "Src/Actor/Character/Player/Player.h"

namespace nsApp
{
	namespace nsActor
	{
		void PlayerCombat::UpdateAction()
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* インタラクト: 足元に落ちている物資を拾う。*/
			if (pPlayer_->GetIntent().bUseTrigger_)
				pPlayer_->PickUpItem();

			/* ライトのON/OFF切り替え。*/
			if (pPlayer_->GetIntent().bLightTrigger_)
				pPlayer_->ToggleLightOn();

			/* メニュー・ポーズ画面。*/
			if (pPlayer_->GetIntent().bPauseTrigger_)
			{
				/* TODO: ポーズ画面/メニューを開く処理に差し替える。*/
				OutputDebugStringA("[Player] Pause!\n");
			}
		}
	}
}