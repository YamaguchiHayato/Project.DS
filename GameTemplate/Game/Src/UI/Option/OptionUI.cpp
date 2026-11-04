#include "stdafx.h"
#include "OptionUI.h"
#include "OptionStyle.h"
#include "Src/UI/KeyConfig/KeyConfigUI.h"

namespace
{
	/**
	 * @brief キーが今押されているかを返す。
	 * @param key 仮想キー。
	 * @return 押されていれば true。
	 */
	bool IsDown(int key)
	{
		return (GetAsyncKeyState(key) & 0x8000) != 0;
	}

	/**
	 * @brief 押した瞬間かを返す。
	 * @param now 今押されているか。
	 * @param was 前フレームで押されていたか。
	 * @return 押した瞬間なら true。
	 */
	bool IsTrigger(bool now, bool was)
	{
		return now && !was;
	}
}

namespace nsApp
{
	namespace nsUI
	{
		OptionUI::~OptionUI()
		{
			/* キー設定が開いたままなら破棄する。 */
			if (pKeyConfig_ != nullptr)
			{
				DeleteGO(pKeyConfig_);
				pKeyConfig_ = nullptr;
			}
		}


		bool OptionUI::Start()
		{
			canvas_.Start();
			FillRows();

			/* 開いた瞬間に押しっぱなしのキーを、新しく押したものとして扱わない。 */
			wasW_ = IsDown('W') || IsDown(VK_UP);
			wasS_ = IsDown('S') || IsDown(VK_DOWN);
			wasA_ = IsDown('A') || IsDown(VK_LEFT);
			wasD_ = IsDown('D') || IsDown(VK_RIGHT);
			wasQ_ = IsDown('Q');
			wasE_ = IsDown('E');
			wasR_ = IsDown('R');
			wasEnter_ = IsDown(VK_RETURN);
			wasEscape_ = IsDown(VK_ESCAPE);
			return true;
		}


		void OptionUI::Update()
		{
			FillRows();

			/* キー設定が開いているあいだは、設定画面の入力を止める。 */
			if (UpdateKeyConfig())
				return;

			UpdateKeyboard();
			UpdateMouse();
		}


		void OptionUI::Render(RenderContext& rc)
		{
			FillRows();

			/* 枠を先に登録する。膜も枠の1枚目として描くので、タブや行より下になる。 */
			canvas_.Begin();
			canvas_.DrawBox({ 0.0f, 0.0f },{ static_cast<float>(FRAME_BUFFER_W), static_cast<float>(FRAME_BUFFER_H) },nsOptionStyle::DIM_COLOR);
			tabBar_.Draw(rc, canvas_, tab_);
			rowList_.Draw(rc, canvas_, model_, rows_, rowCount_, top_, cursor_);

			const OptionEntry* selected = nullptr;
			if (cursor_ >= 0 && cursor_ < rowCount_)
				selected = rows_[cursor_];
			footer_.Draw(rc, canvas_, selected);
		}


		void OptionUI::FillRows()
		{
			rowCount_ = 0;
			for (int index = 0; index < OPTION_ENTRY_COUNT; index++)
			{
				if (OPTION_ENTRIES[index].tab != tab_)
					continue;
				if (rowCount_ >= MAX_TAB_ROWS)
					break;
				rows_[rowCount_] = &OPTION_ENTRIES[index];
				rowCount_++;
			}
			ClampCursor();
		}


		void OptionUI::ClampCursor()
		{
			if (rowCount_ <= 0)
			{
				cursor_ = 0;
				top_ = 0;
				return;
			}

			if (cursor_ >= rowCount_)
				cursor_ = rowCount_ - 1;
			if (cursor_ < 0)
				cursor_ = 0;

			/* 選んでいる行が画面の外に出ないように、一番上の行を動かす。 */
			if (cursor_ < top_)
				top_ = cursor_;

			if (cursor_ >= top_ + OptionRowList::MAX_VISIBLE)
				top_ = cursor_ - OptionRowList::MAX_VISIBLE + 1;
		}


		void OptionUI::ChangeTab(OptionTab tab)
		{
			tab_ = tab;
			cursor_ = 0;
			top_ = 0;
			FillRows();
		}


		void OptionUI::StepCurrent(int direction)
		{
			if (cursor_ < 0 || cursor_ >= rowCount_)
				return;

			model_.Step(*rows_[cursor_], direction);
		}


		void OptionUI::OpenKeyConfig()
		{
			if (pKeyConfig_ != nullptr)
				return;

			pKeyConfig_ = NewGO<KeyConfigUI>(1, "keyConfig");
		}


		bool OptionUI::UpdateKeyConfig()
		{
			if (pKeyConfig_ == nullptr)
				return false;

			if (pKeyConfig_->IsClose())
			{
				DeleteGO(pKeyConfig_);
				pKeyConfig_ = nullptr;

				/* 閉じたフレームの Escape で、設定画面まで閉じない。 */
				wasEscape_ = true;
				return false;
			}
			return true;
		}


		void OptionUI::UpdateKeyboard()
		{
			const bool w = IsDown('W') || IsDown(VK_UP);
			const bool s = IsDown('S') || IsDown(VK_DOWN);
			const bool a = IsDown('A') || IsDown(VK_LEFT);
			const bool d = IsDown('D') || IsDown(VK_RIGHT);
			const bool q = IsDown('Q');
			const bool e = IsDown('E');
			const bool r = IsDown('R');
			const bool enter = IsDown(VK_RETURN);
			const bool escape = IsDown(VK_ESCAPE);

			if (IsTrigger(escape, wasEscape_))
			{
				nsData::GraphicsSetting::Get().Save();
				close_ = true;
			}

			if (IsTrigger(q, wasQ_))
			{
				const int tab = static_cast<int>(tab_);
				const int count = static_cast<int>(OptionTab::Count);
				ChangeTab(static_cast<OptionTab>((tab + count - 1) % count));
			}
			if (IsTrigger(e, wasE_))
			{
				const int tab = static_cast<int>(tab_);
				const int count = static_cast<int>(OptionTab::Count);
				ChangeTab(static_cast<OptionTab>((tab + 1) % count));
			}

			if (IsTrigger(w, wasW_) && cursor_ > 0)
			{
				cursor_--;
				ClampCursor();
			}
			if (IsTrigger(s, wasS_) && cursor_ < rowCount_ - 1)
			{
				cursor_++;
				ClampCursor();
			}

			if (IsTrigger(a, wasA_))
				StepCurrent(-1);

			if (IsTrigger(d, wasD_))
				StepCurrent(1);

			if (IsTrigger(r, wasR_) && cursor_ >= 0 && cursor_ < rowCount_)
				model_.Reset(*rows_[cursor_]);

			if (IsTrigger(enter, wasEnter_) && cursor_ >= 0 && cursor_ < rowCount_)
			{
				const OptionEntry& entry = *rows_[cursor_];
				if (entry.type == OptionRowType::Command && static_cast<OptionCommand>(entry.item) == OptionCommand::KeyConfig)
					OpenKeyConfig();
			}

			wasW_ = w;
			wasS_ = s;
			wasA_ = a;
			wasD_ = d;
			wasQ_ = q;
			wasE_ = e;
			wasR_ = r;
			wasEnter_ = enter;
			wasEscape_ = escape;
		}


		void OptionUI::UpdateMouse()
		{
			if (g_mouse == nullptr)
				return;

			const int wheel = g_mouse->GetWheelDelta();
			if (wheel > 0 && cursor_ > 0)
			{
				cursor_--;
				ClampCursor();
			}
			if (wheel < 0 && cursor_ < rowCount_ - 1)
			{
				cursor_++;
				ClampCursor();
			}

			if (!g_mouse->IsTrigger(enMouseButtonLeft))
				return;

			const Vector2 point = g_mouse->GetSpritePos();

			const int tab = tabBar_.HitTab(point);
			if (tab >= 0)
			{
				ChangeTab(static_cast<OptionTab>(tab));
				return;
			}

			const int tabArrow = tabBar_.HitArrow(point);
			if (tabArrow != 0)
			{
				const int now = static_cast<int>(tab_);
				const int count = static_cast<int>(OptionTab::Count);
				ChangeTab(static_cast<OptionTab>((now + count + tabArrow) % count));
				return;
			}

			const int rowArrow = rowList_.HitArrow(point, top_, cursor_);
			if (rowArrow != 0)
			{
				StepCurrent(rowArrow);
				return;
			}

			const int row = rowList_.HitRow(point, rowCount_, top_);
			if (row < 0)
				return;

			if (row != cursor_)
			{
				cursor_ = row;
				ClampCursor();
				return;
			}

			/* 選んでいる行をもう一度クリックした。選択肢は切り替え、キー設定は開く。 */
			const OptionEntry& entry = *rows_[cursor_];
			if (entry.type == OptionRowType::Choice)
			{
				StepCurrent(1);
				return;
			}
			if (entry.type == OptionRowType::Command && static_cast<OptionCommand>(entry.item) == OptionCommand::KeyConfig)
				OpenKeyConfig();
		}
	}
}