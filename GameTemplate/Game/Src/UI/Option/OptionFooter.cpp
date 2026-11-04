#include "stdafx.h"
#include "OptionFooter.h"
#include "OptionCanvas.h"
#include "OptionStyle.h"

namespace
{
	const Vector2 DESCRIPTION_POSITION = { 0.0f, -350.0f }; //! 説明文の中心。
	const float DESCRIPTION_SCALE = 0.5f; //! 説明文の大きさ。

	const wchar_t* GUIDE_TEXT = L"W/S 選択   A/D 変更   Q/E タブ   R 初期化   Esc 戻る"; //! 操作ガイド。
	const Vector2 GUIDE_POSITION = { 900.0f, -500.0f }; //! 操作ガイドの右端。
	const float GUIDE_SCALE = 0.4f; //! 操作ガイドの大きさ。
}


namespace nsApp
{
	namespace nsUI
	{
		void OptionFooter::Draw(RenderContext& rc, OptionCanvas& canvas, const OptionEntry* entry)const
		{
			/* 選んでいる行の説明。 */
			if (entry != nullptr)
			{
				canvas.DrawString(rc, entry->description, DESCRIPTION_POSITION, DESCRIPTION_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Center);
			}

			/* 右下の操作ガイド。 */
			canvas.DrawString(rc, GUIDE_TEXT, GUIDE_POSITION, GUIDE_SCALE, nsOptionStyle::TEXT_LIGHT, TextAlign::Right);
		}
	}
}