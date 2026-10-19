#include "stdafx.h"
#include "PlayerStateBase.h"
#include "Src/Actor/Character/Player/Player.h"

namespace nsApp
{
	namespace nsActor
	{
		void PlayerStateBase::Enter()
		{
			/* 種類に依存しない口へ、入室時に一度だけ寄せる。*/
			pPlayer_ = static_cast<Player*>(pOwner_);

			/* 樹の現在節をこのステート種別に同期する。*/
			pPlayer_->NotifyPlayerState(GetStateKind());

			/* ステート固有の入室処理。*/
			OnEnter();
		}


		void PlayerStateBase::Update()
		{
			/* 遷移が必要なら切り替えて、このフレームの仕事はしない。*/
			if (pPlayer_->TryChangeState())
				return;

			/* ステート固有の仕事。*/
			OnUpdate();
		}


		void PlayerStateBase::Exit()
		{
			/* 所有者キャッシュを捨てる。*/
			pPlayer_ = nullptr;
		}
	}
}