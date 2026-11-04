#include "stdafx.h"
#include "OptionModel.h"
#include <algorithm>
#include <cmath>

namespace nsApp
{
	namespace nsUI
	{
		OptionValue OptionModel::Read(const OptionEntry& entry)const
		{
			/* 別画面を開く行は値を持たない。 */
			const float raw = nsData::GraphicsSetting::Get().GetValue(entry.category, entry.item);

			/* 選択肢は番号、それ以外は数値として読む。 */
			OptionValue value = {};
			if (entry.type == OptionRowType::Choice)
				value.index = static_cast<int>(raw + 0.5f);
			else
				value.number = raw;

			return value;
		}


		void OptionModel::Step(const OptionEntry& entry, int direction)const
		{
			/* 別画面を開く行は値を持たない。 */
			if (entry.type == OptionRowType::Command)
				return;

			nsData::GraphicsSetting& setting = nsData::GraphicsSetting::Get();
			const nsData::SettingRange& range = setting.GetRange(entry.category, entry.item);

			/* 選択肢は端で止めず、左右どちらでも次へ送る。ミュートの ON/OFF が1回で切り替わる。 */
			if (entry.type == OptionRowType::Choice)
			{
				/* 選択肢の数を計算する。 */
				const int last = static_cast<int>((range.max - range.min) / range.step + 0.5f);
				int index = static_cast<int>(setting.GetValue(entry.category, entry.item) + 0.5f);
				index += direction;

				/* 端を超えたら反対側へ送る。 */
				if (index < 0)
					index = last;
				if (index > last)
					index = 0;
				setting.SetValue(entry.category, entry.item, range.min + static_cast<float>(index) * range.step);
				return;
			}

			/* ゲージは刻みの何個目かに直してから動かす。0.1 を足し続けたときの誤差をためない。 */
			const float now = setting.GetValue(entry.category, entry.item);
			const float steps = std::round((now - range.min) / range.step) + static_cast<float>(direction);
			setting.SetValue(entry.category, entry.item, range.min + steps * range.step);
		}


		void OptionModel::Reset(const OptionEntry& entry)const
		{
			/* 別画面を開く行は値を持たない。 */
			if (entry.type == OptionRowType::Command)
				return;

			nsData::GraphicsSetting& setting = nsData::GraphicsSetting::Get();
			const nsData::SettingRange& range = setting.GetRange(entry.category, entry.item);
			setting.SetValue(entry.category, entry.item, range.defaultValue);
		}


		float OptionModel::GetRatio(const OptionEntry& entry)const
		{
			nsData::GraphicsSetting& setting = nsData::GraphicsSetting::Get();
			const nsData::SettingRange& range = setting.GetRange(entry.category, entry.item);
			const float width = range.max - range.min;

			/* 幅が無い項目はゲージを出さない。 */
			if (width <= 0.0f)
				return 0.0f;

			const float now = setting.GetValue(entry.category, entry.item);
			return std::clamp((now - range.min) / width, 0.0f, 1.0f);
		}


		void OptionModel::Format(const OptionEntry& entry, wchar_t* text, size_t count)const
		{
			const OptionValue value = Read(entry);

			switch (entry.type)
			{
			case OptionRowType::Gauge:
				/* 値に倍率を掛けて整数で出す。 */
				swprintf_s(text, count, L"%d", static_cast<int>(std::lround(value.number * entry.displayScale)));
				break;

			case OptionRowType::Choice:
				/* 番号に対応する表示名を出す。 */
				if (entry.choices != nullptr)
					wcscpy_s(text, count, entry.choices[value.index]);
				else
					text[0] = L'\0';
				break;

			default:
				text[0] = L'\0';
				break;
			}
		}
	}
}