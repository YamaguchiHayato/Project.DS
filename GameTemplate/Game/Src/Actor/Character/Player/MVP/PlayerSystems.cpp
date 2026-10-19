#include "stdafx.h"
#include "PlayerSystems.h"


namespace nsApp
{
	namespace nsActor
	{
		void PlayerSystems::Init(Player* pPlayer, const nsData::PlayerStatus& status, const Vector3& position)
		{
			/* 各部位の初期化をまとめる。*/
			Initializers(pPlayer, status, position);
		}


		void PlayerSystems::Tick(float deltaTime, const PlayerIntent& intent)
		{
			/* 常時更新処理をまとめる。*/
			AlwaysUpdates(deltaTime, intent);

			/* 更新処理をまとめる。*/
			Updaters(deltaTime);
		}


		void PlayerSystems::Present()
		{
			/* 見た目反映処理をまとめる。*/
			Presenters();
		}


		void PlayerSystems::Render(RenderContext& rc)
		{
			view_.RenderBody(rc);
			view_.RenderWeapon(rc);
		}


		void PlayerSystems::Initializers(Player* pPlayer, const nsData::PlayerStatus& status, const Vector3& position)
		{
			/* 各部位を初期化する。*/
			view_.Bind(pPlayer);
			view_.Init(status, position);
			update_.Bind(pPlayer);
			calc_.Init();
			combat_.Init();
			item_.Init();
		}



		void PlayerSystems::AlwaysUpdates(float deltaTime, const PlayerIntent& intent)
		{
			/* 視点・ADS・体力・移動など、毎フレ進める処理。*/
			update_.UpdateLook(intent);
			update_.UpdateAds(deltaTime, intent);
			update_.UpdateStanceSpread(deltaTime);
			update_.UpdateRecoil(deltaTime);
			update_.UpdateHealth(deltaTime);
			update_.UpdateEyeHeight(deltaTime);
			update_.UpdateLifeState(deltaTime);
			update_.UpdateMove(deltaTime, intent);
			update_.UpdateViewSway(deltaTime, intent);
		}


		void PlayerSystems::Updaters(float deltaTime)
		{
			/* 戦闘・アイテムなど、操作意図に応じて進める処理。*/
			combat_.UpdateWeapon(deltaTime);
			combat_.UpdateShove(deltaTime);
			combat_.UpdateAction();
			item_.UpdateItems(deltaTime);
		}


		void PlayerSystems::Presenters()
		{
			/* 見た目を反映する。*/
			view_.Present();
			view_.PresentWeapon();
			view_.PresentFlashLight();
		}
	}
}