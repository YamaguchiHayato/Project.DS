#include "stdafx.h"
#include "GameFlow.h"
#include "InGameScene.h"
#include "Src/Scene/TitleScene.h"

namespace
{
	const uint32_t uSceneShift_ = 8;
}

namespace nsApp
{
	namespace nsScene
	{
		bool GameFlow::Start()
		{
			/* シーン生成関数を登録する。*/
			RegisterSceneFactory();

			/* 起動時はタイトルへ遷移する。*/
			enReserveSceneID_ = EnSceneID::Title;
			ApplyReservedSceneChange();
			return true;
		}


		void GameFlow::Update()
		{
			/* 予約が無い場合は何もしない。*/
			if (enReserveSceneID_ == EnSceneID::None)
				return;

			/* 予約されたシーン切替を反映する。*/
			ApplyReservedSceneChange();
		}


		uint32_t GameFlow::MakeSceneKey(EnSceneID enSceneID, EnDebugSceneID enDebugSceneID) const
		{
			/* SceneID と DebugSceneID からキーを作る。*/
			return (static_cast<uint32_t>(enSceneID) << uSceneShift_) | static_cast<uint32_t>(enDebugSceneID);
		}


		void GameFlow::RegisterSceneFactory()
		{
			/* タイトルシーン。*/
			mapSceneFactory_[MakeSceneKey(EnSceneID::Title, EnDebugSceneID::None)] = [](GameFlow* pGameFlow) -> IScene*
				{
					TitleScene* pScene = NewGO<TitleScene>(0, "titleScene");
					pScene->SetGameFlow(pGameFlow);
					return pScene;
				};

			/* InGame / Debug はクラス作成後に登録する。*/
			mapSceneFactory_[MakeSceneKey(EnSceneID::InGame, EnDebugSceneID::None)] = [](GameFlow* pGameFlow) -> IScene*
				{
					InGameScene* pScene = NewGO<InGameScene>(0, "inGameScene");
					pScene->SetGameFlow(pGameFlow);
					return pScene;
				};
		}


		void GameFlow::ApplyReservedSceneChange()
		{
			const uint32_t uKey = MakeSceneKey(enReserveSceneID_, enReserveDebugSceneID_);

			/* 未登録キーなら予約だけクリアする。*/
			if (mapSceneFactory_.count(uKey) == 0)
			{
				enReserveSceneID_ = EnSceneID::None;
				enReserveDebugSceneID_ = EnDebugSceneID::None;
				return;
			}

			/* 現在シーンを破棄する。*/
			if (pCurrentScene_ != nullptr)
			{
				DeleteGO(pCurrentScene_);
				pCurrentScene_ = nullptr;
			}

			/* 新しいシーンを生成する。*/
			pCurrentScene_ = mapSceneFactory_[uKey](this);

			/* 予約をクリアする。*/
			enReserveSceneID_ = EnSceneID::None;
			enReserveDebugSceneID_ = EnDebugSceneID::None;
		}
	}
}