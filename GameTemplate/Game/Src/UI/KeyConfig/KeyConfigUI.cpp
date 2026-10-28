#include "stdafx.h"
#include "KeyConfigUI.h"
#include "Src/InputObserver.h"

namespace nsApp
{
	namespace nsUI
	{
		bool KeyConfigUI::Start()
		{
			/* InputObserver から操作の数を取って、フォントを描画する。*/
			nsData::InputObserver& input_ = nsData::InputObserver::Get();
			const int count_ = input_.GetActionCount();
			float y_ = 460.0f;

			for (int index_ = 0; index_ < count_ && index_ < fontCount_; index_++)
			{
				wchar_t line_[96];
				swprintf_s(line_,L"%hs    %hs",input_.GetActionLabel(index_),input_.GetBindingLabel(index_).c_str());

				font_[index_].SetPosition({ -420.0f, y_, 0.0f });
				font_[index_].SetScale(0.7f);
				font_[index_].SetColor(1.0f, 1.0f, 1.0f, 1.0f);
				font_[index_].SetText(line_);
				y_ -= 34.0f;
			}
			return true;
		}


		void KeyConfigUI::Update()
		{
			nsData::InputObserver& input_ = nsData::InputObserver::Get();
			const int count_ = input_.GetActionCount();
			const bool up_ = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
			const bool down_ = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
			const bool enter_ = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
			const bool back_ = (GetAsyncKeyState(VK_BACK) & 0x8000) != 0;
			const bool escape_ = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
			if (escape_ && !wasEscape_)
				close_ = true;

			/* 入力待ちのあいだは、行を動かさない。*/
			if (!waiting_)
			{
				if (up_ && !wasUp_ && selected_ > 0)
					selected_--;
				if (down_ && !wasDown_ && selected_ < count_ - 1)
					selected_++;

				/* Enterを押した瞬間だけ、次のキー待ちに入る。*/
				if (enter_ && !wasEnter_)
					waiting_ = true;

				if (back_ && !wasBack_)
				{
					if (input_.Reset())
					{
						for (int index_ = 0; index_ < count_ && index_ < fontCount_; index_++)
						{
							wchar_t line_[96];
							swprintf_s(line_,L"%hs    %hs",input_.GetActionLabel(index_),input_.GetBindingLabel(index_).c_str());
							font_[index_].SetText(line_);
						}
					}
				}

			}
			else if (!enter_)
			{
				/* Enterを離したあと、次に押されたキーをその行へ入れる。*/
				if (input_.Rebind(selected_))
				{
					wchar_t line_[96];
					swprintf_s(line_, L"%hs    %hs",input_.GetActionLabel(selected_), input_.GetBindingLabel(selected_).c_str());
					font_[selected_].SetText(line_);
					waiting_ = false;
				}
			}

			wasUp_ = up_;
			wasDown_ = down_;
			wasEnter_ = enter_;
			wasBack_ = back_;
			wasEscape_ = escape_;

			for (int index_ = 0; index_ < count_ && index_ < fontCount_; index_++)
			{
				if (index_ == selected_)
					font_[index_].SetColor(1.0f, 0.9f, 0.2f, 1.0f);
				else
					font_[index_].SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}


		void KeyConfigUI::Render(RenderContext& rc)
		{
			/* InputObserver から操作の数を取って、フォントを描画する。*/
			const int count_ = nsData::InputObserver::Get().GetActionCount();

			/* フォントの数は fontCount_ で制限されているので、両方の小さい方を使う。*/
			for (int index_ = 0; index_ < count_ && index_ < fontCount_; index_++)
				font_[index_].Draw(rc);
		}
	}
}