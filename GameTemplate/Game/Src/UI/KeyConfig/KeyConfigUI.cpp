#include "stdafx.h"
#include "KeyConfigUI.h"
#include "Src/UI/Option/OptionStyle.h"
#include "Src/InputObserver.h"

namespace
{
	/**
	 * @brief 表の1行。見出しか、付け替えできる操作か。
	 */
	struct KeyConfigRow
	{
		bool isHeader;
		unsigned char actionIndex;
		const wchar_t* label;
	};

	const KeyConfigRow ROWS[] =
	{
		{ true, 0, L"移動" },
		{ false, 0, L"前進" },
		{ false, 1, L"後退" },
		{ false, 2, L"左移動" },
		{ false, 3, L"右移動" },
		{ false, 15, L"走る" },
		{ false, 16, L"しゃがみ" },

		{ true, 0, L"戦闘" },
		{ false, 4, L"射撃" },
		{ false, 5, L"覗き込み" },
		{ false, 8, L"リロード" },
		{ false, 17, L"突き飛ばす" },
		{ false, 18, L"メイン武器" },
		{ false, 19, L"サブ武器" },
		{ false, 6, L"次の武器" },
		{ false, 7, L"前の武器" },

		{ true, 0, L"アクション" },
		{ false, 9, L"調べる" },
		{ false, 10, L"ライト" },
		{ false, 12, L"回復" },
		{ false, 13, L"投げる" },
		{ false, 14, L"クイックアイテム" },
		{ false, 21, L"視点切替" },

		{ true, 0, L"メニュー" },
		{ false, 11, L"ポーズ" },
		{ false, 20, L"決定" },
	};

	const int ROW_COUNT = static_cast<int>(sizeof(ROWS) / sizeof(ROWS[0]));
	const int MAX_VISIBLE = 9; //! 一度に見せる行の数。設定画面と同じ。

	const wchar_t* TITLE_TEXT = L"キー設定";
	const Vector2 TITLE_POSITION = { -900.0f, 480.0f };
	const float TITLE_SCALE = 0.8f;
	const float LINE_Y = 392.0f;
	const Vector2 LINE_SIZE = { 1760.0f, 4.0f };

	const float ROW_TOP_Y = 300.0f;
	const float ROW_STEP_Y = -48.0f;
	const float ROW_TEXT_SCALE = 0.5f;
	const Vector2 BAND_SIZE = { 1020.0f, 38.0f };
	const float LABEL_BOX_X = -345.0f;
	const Vector2 LABEL_BOX_SIZE = { 330.0f, 34.0f };
	const float LABEL_TEXT_X = -495.0f;
	const float VALUE_X = 290.0f;
	const int VALUE_TEXT_LENGTH = 32;

	const wchar_t* GUIDE_TEXT = L"W/S 選択   Enter 変更   R 初期化   Esc 戻る";
	const Vector2 GUIDE_POSITION = { 900.0f, -500.0f };
	const float GUIDE_SCALE = 0.4f;

	const wchar_t* WAITING_TEXT = L"入力待ち";

	/**
	 * @brief 割り当て名を、表の右側に出す文字にする。
	 */
	void FormatBinding(const char* name, wchar_t* outText, int outLength)
	{
		if (strcmp(name, "MouseLeft") == 0)
			swprintf_s(outText, outLength, L"左クリック");
		else if (strcmp(name, "MouseRight") == 0)
			swprintf_s(outText, outLength, L"右クリック");
		else if (strcmp(name, "MouseMiddle") == 0)
			swprintf_s(outText, outLength, L"ホイールクリック");
		else if (strcmp(name, "WheelUp") == 0)
			swprintf_s(outText, outLength, L"ホイール上");
		else if (strcmp(name, "WheelDown") == 0)
			swprintf_s(outText, outLength, L"ホイール下");
		else if (strcmp(name, "Control") == 0)
			swprintf_s(outText, outLength, L"Ctrl");
		else if (strcmp(name, "Escape") == 0)
			swprintf_s(outText, outLength, L"Esc");
		else if (strcmp(name, "Return") == 0)
			swprintf_s(outText, outLength, L"Enter");
		else
		{
			size_t converted = 0;
			mbstowcs_s(&converted, outText, static_cast<size_t>(outLength), name, _TRUNCATE);
		}
	}

	float GetRowY(int visible)
	{
		return ROW_TOP_Y + ROW_STEP_Y * static_cast<float>(visible);
	}

	bool IsDown(int key)
	{
		return (GetAsyncKeyState(key) & 0x8000) != 0;
	}

	bool IsTrigger(bool now, bool was)
	{
		return now && !was;
	}

	int FirstSelectable()
	{
		for (int index = 0; index < ROW_COUNT; index++)
		{
			if (!ROWS[index].isHeader)
				return index;
		}
		return 0;
	}

	int HitRow(const Vector2& point, int top)
	{
		for (int visible = 0; visible < MAX_VISIBLE; visible++)
		{
			const int index = top + visible;
			if (index >= ROW_COUNT)
				break;
			if (ROWS[index].isHeader)
				continue;
			if (nsApp::nsUI::nsOptionStyle::IsInside(point, { 0.0f, GetRowY(visible) }, BAND_SIZE))
				return index;
		}
		return -1;
	}
}

namespace nsApp
{
	namespace nsUI
	{
		bool KeyConfigUI::Start()
		{
			canvas_.Start();
			selected_ = FirstSelectable();
			top_ = 0;
			ClampTop();

			/*
			 * 開いた瞬間に押しっぱなしのキーを、新しく押したものとして扱わない。
			 * 設定の「キー設定」を Enter で選ぶと、その Enter がまだ押されたままここへ来る。
			 * 前フレームを「押されていない」のままにすると、開いた瞬間に割り当て待ちへ入り、
			 * 閉じようとして押した Esc がそのまま割り当てられてしまう。
			 */
			wasW_ = IsDown('W') || IsDown(VK_UP);
			wasS_ = IsDown('S') || IsDown(VK_DOWN);
			wasEnter_ = IsDown(VK_RETURN);
			wasR_ = IsDown('R');
			wasEscape_ = IsDown(VK_ESCAPE);
			wasMouseLeft_ = g_mouse != nullptr && g_mouse->IsPress(enMouseButtonLeft);
			return true;
		}


		void KeyConfigUI::Update()
		{
			UpdateKeyboard();
			UpdateMouse();
		}


		void KeyConfigUI::Render(RenderContext& rc)
		{
			nsData::InputObserver& input = nsData::InputObserver::Get();

			canvas_.Begin();
			canvas_.DrawBox({ 0.0f, 0.0f }, { static_cast<float>(FRAME_BUFFER_W), static_cast<float>(FRAME_BUFFER_H) }, nsOptionStyle::DIM_COLOR);
			canvas_.DrawString(rc, TITLE_TEXT, TITLE_POSITION, TITLE_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Left);
			canvas_.DrawBox({ 0.0f, LINE_Y }, LINE_SIZE, nsOptionStyle::LINE_COLOR);

			for (int visible = 0; visible < MAX_VISIBLE; visible++)
			{
				const int index = top_ + visible;
				if (index >= ROW_COUNT)
					break;

				const KeyConfigRow& row = ROWS[index];
				const float y = GetRowY(visible);

				if (row.isHeader)
				{
					canvas_.DrawString(rc, row.label, { LABEL_TEXT_X, y }, ROW_TEXT_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Left);
					continue;
				}

				const bool isSelected = index == selected_;
				const Vector4& textColor = isSelected ? nsOptionStyle::TEXT_DARK : nsOptionStyle::TEXT_LIGHT;

				if (isSelected)
					canvas_.DrawBox({ 0.0f, y }, BAND_SIZE, nsOptionStyle::SELECT_COLOR);
				else
					canvas_.DrawBox({ LABEL_BOX_X, y }, LABEL_BOX_SIZE, nsOptionStyle::IDLE_COLOR);

				canvas_.DrawString(rc, row.label, { LABEL_TEXT_X, y }, ROW_TEXT_SCALE, textColor, TextAlign::Left);

				wchar_t value[VALUE_TEXT_LENGTH];
				if (isSelected && waiting_)
					canvas_.DrawString(rc, WAITING_TEXT, { VALUE_X, y }, ROW_TEXT_SCALE, nsOptionStyle::ACCENT_COLOR, TextAlign::Center);
				else
				{
					FormatBinding(input.GetBindingLabel(row.actionIndex).c_str(), value, VALUE_TEXT_LENGTH);
					canvas_.DrawString(rc, value, { VALUE_X, y }, ROW_TEXT_SCALE, textColor, TextAlign::Center);
				}
			}

			canvas_.DrawString(rc, GUIDE_TEXT, GUIDE_POSITION, GUIDE_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Right);
		}


		void KeyConfigUI::MoveCursor(int direction)
		{
			int next = selected_ + direction;
			while (next >= 0 && next < ROW_COUNT && ROWS[next].isHeader)
				next += direction;

			if (next < 0 || next >= ROW_COUNT || ROWS[next].isHeader)
				return;

			selected_ = next;
			ClampTop();
		}


		void KeyConfigUI::ClampTop()
		{
			if (selected_ < top_)
				top_ = selected_;
			if (selected_ >= top_ + MAX_VISIBLE)
				top_ = selected_ - MAX_VISIBLE + 1;

			/* 見出しが一番上に残るように、範囲だけ直す。 */
			if (top_ < 0)
				top_ = 0;
			if (top_ > ROW_COUNT - 1)
				top_ = ROW_COUNT - 1;
		}


		void KeyConfigUI::UpdateKeyboard()
		{
			const bool w = IsDown('W') || IsDown(VK_UP);
			const bool s = IsDown('S') || IsDown(VK_DOWN);
			const bool enter = IsDown(VK_RETURN);
			const bool r = IsDown('R');
			const bool escape = IsDown(VK_ESCAPE);
			const bool mouseLeft = g_mouse != nullptr && g_mouse->IsPress(enMouseButtonLeft);

			/* 割り当て待ちの最中は Esc も割り当て対象なので、閉じる操作にしない。 */
			if (escape && !wasEscape_ && !waiting_)
				close_ = true;

			if (!waiting_)
			{
				if (IsTrigger(w, wasW_))
					MoveCursor(-1);
				if (IsTrigger(s, wasS_))
					MoveCursor(1);

				if (IsTrigger(enter, wasEnter_))
					waiting_ = true;

				if (IsTrigger(r, wasR_))
					nsData::InputObserver::Get().Reset();
			}
			else if (!enter && !mouseLeft)
			{
				/* Enter とクリックを離したあと、次に押されたキーをその行へ入れる。 */
				TryRebind();
			}

			wasW_ = w;
			wasS_ = s;
			wasEnter_ = enter;
			wasR_ = r;
			wasEscape_ = escape;
			wasMouseLeft_ = mouseLeft;
		}


		void KeyConfigUI::UpdateMouse()
		{
			if (waiting_ || g_mouse == nullptr)
				return;

			const int wheel = g_mouse->GetWheelDelta();
			if (wheel > 0)
				MoveCursor(-1);
			if (wheel < 0)
				MoveCursor(1);

			if (!g_mouse->IsTrigger(enMouseButtonLeft))
				return;

			const int row = HitRow(g_mouse->GetSpritePos(), top_);
			if (row < 0)
				return;

			if (row != selected_)
			{
				selected_ = row;
				ClampTop();
				return;
			}

			/* 選んでいる行をもう一度クリックしたので、割り当て待ちへ入る。 */
			waiting_ = true;
		}


		void KeyConfigUI::TryRebind()
		{
			const KeyConfigRow& row = ROWS[selected_];
			if (row.isHeader)
			{
				waiting_ = false;
				return;
			}

			if (nsData::InputObserver::Get().Rebind(row.actionIndex))
				waiting_ = false;
		}
	}
}