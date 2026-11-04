#include "stdafx.h"
#include "MenuScene.h"
#include "system/system.h"
#include "Src/Scene/GameFlow.h"
#include "Src/UI/Title/TitleUI.h"
#include "Src/UI/Title/TitleStage.h"
#include "Src/InputObserver.h"
#pragma comment(lib, "Shell32.lib")

namespace
{
	const wchar_t* REPOSITORY_URL = L"https://github.com/YamaguchiHayato/Project.DS";

	void OpenRepositoryPage()
	{
		ShellExecuteW(nullptr, L"open", REPOSITORY_URL, nullptr, nullptr, SW_SHOWNORMAL);
	}
}


namespace nsApp
{
	namespace nsScene
	{
		MenuScene::~MenuScene()
		{
			if (pMenuUI_ != nullptr)
			{
				DeleteGO(pMenuUI_);
				pMenuUI_ = nullptr;
			}

			if (pTitleStage_ != nullptr)
			{
				DeleteGO(pTitleStage_);
				pTitleStage_ = nullptr;
			}
		}


		bool MenuScene::Start()
		{
			pMenuUI_ = NewGO<nsUI::TitleUI>(0, "menu");
			pMenuUI_->SetOpenAsMenu();

			pTitleStage_ = NewGO<nsUI::TitleStage>(0, "titleStage");
			pTitleStage_->SetVisible(true);
			return true;
		}


		void MenuScene::Update()
		{
			if (pMenuUI_ != nullptr)
				ApplyMenuCommand(pMenuUI_->PopDecidedCommand());

			if (nsData::InputObserver::Get().IsTrigger(nsData::EnAction::OpenDebug))
				pGameFlow_->ChangeScene(EnSceneID::Debug);
		}


		void MenuScene::ApplyMenuCommand(nsUI::EnTitleCommand enCommand)
		{
			switch (enCommand)
			{
			case nsUI::EnTitleCommand::StartGame:
				pGameFlow_->ChangeScene(EnSceneID::InGame);
				break;

			case nsUI::EnTitleCommand::Option:
				pGameFlow_->ChangeScene(EnSceneID::Option);
				break;

			case nsUI::EnTitleCommand::OpenGitHub:
				OpenRepositoryPage();
				break;

			case nsUI::EnTitleCommand::ExitGame:
				g_gameLoop.m_isLoop = false;
				break;

			default:
				break;
			}
		}
	}
}