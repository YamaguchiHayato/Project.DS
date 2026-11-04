#include "stdafx.h"
#include "OptionScene.h"
#include "Src/Scene/GameFlow.h"
#include "Src/UI/Option/OptionUI.h"

namespace nsApp
{
	namespace nsScene
	{
		OptionScene::~OptionScene()
		{
			/* 生成した設定画面を破棄する。 */
			if (pOptionUI_ != nullptr)
			{
				DeleteGO(pOptionUI_);
				pOptionUI_ = nullptr;
			}
		}


		bool OptionScene::Start()
		{
			/* 設定の表示を生成する。 */
			pOptionUI_ = NewGO<nsUI::OptionUI>(0, "option");
			return true;
		}


		void OptionScene::Update()
		{
			/* まだ開いていなければ何もしない。 */
			if (pOptionUI_ == nullptr)
				return;

			/* 閉じられるまでは開いたままにする。 */
			if (!pOptionUI_->IsClose())
				return;

			/* 閉じたのでメニューへ戻る。保存は OptionUI 側で済んでいる。 */
			pGameFlow_->ChangeScene(EnSceneID::Menu);
		}
	}
}