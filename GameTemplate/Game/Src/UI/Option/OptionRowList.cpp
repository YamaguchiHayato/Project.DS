#include "stdafx.h"
#include "OptionRowList.h"
#include "OptionCanvas.h"
#include "OptionModel.h"
#include "OptionStyle.h"

namespace
{
	const float ROW_TOP_Y = 300.0f; //! 一番上の行の縦の位置。
	const float ROW_STEP_Y = -48.0f; //! 行どうしの間隔。
	const float ROW_TEXT_SCALE = 0.5f; //! 行の文字の大きさ。

	const Vector2 BAND_CENTER_X_SIZE = { 1020.0f, 38.0f }; //! 選択中の帯の大きさ。中心は画面の中央。
	const float LABEL_BOX_X = -345.0f; //! 項目名の地の中心の横位置。
	const Vector2 LABEL_BOX_SIZE = { 330.0f, 34.0f }; //! 項目名の地の大きさ。
	const float LABEL_TEXT_X = -495.0f; //! 項目名の左端。

	const float VALUE_X = 290.0f; //! 選択肢の文字の中心。
	const float GAUGE_NUMBER_X = 140.0f; //! ゲージの数字の右端。
	const float GAUGE_X = 306.0f; //! ゲージの中心の横位置。
	const Vector2 GAUGE_SIZE = { 300.0f, 14.0f }; //! ゲージの大きさ。

	const float ARROW_LEFT_X = 70.0f; //! 左の矢印の横位置。
	const float ARROW_RIGHT_X = 485.0f; //! 右の矢印の横位置。
	const Vector2 ARROW_HIT_SIZE = { 50.0f, 38.0f }; //! 矢印をクリックできる範囲。

	const wchar_t* COMMAND_TEXT = L"開く"; //! 別画面を開く行に出す文字。
	const int VALUE_TEXT_LENGTH = 32; //! 値の文字の長さ。

	/**
	 * @brief 見えている何行目かから、縦の位置を返す。
	 * @param visible 見えている何行目か。
	 * @return 縦の位置。
	 */
	float GetRowY(int visible)
	{
		return ROW_TOP_Y + ROW_STEP_Y * static_cast<float>(visible);
	}
}

namespace nsApp
{
	namespace nsUI
	{
		void OptionRowList::Draw(RenderContext& rc, OptionCanvas& canvas, const OptionModel& model,
			const OptionEntry* const* rows, int rowCount, int top, int cursor)const
		{
			for (int visible = 0; visible < MAX_VISIBLE; visible++)
			{
				const int index = top + visible;
				if (index >= rowCount)
					break;

				const OptionEntry& entry = *rows[index];
				const bool isSelected = index == cursor;
				const float y = GetRowY(visible);
				const Vector4& textColor = isSelected ? nsOptionStyle::TEXT_DARK : nsOptionStyle::TEXT_LIGHT;

				/* 選んでいる行は帯、それ以外は項目名の地。 */
				if (isSelected)
					canvas.DrawBox({ 0.0f, y }, BAND_CENTER_X_SIZE, nsOptionStyle::SELECT_COLOR);

				else
					canvas.DrawBox({ LABEL_BOX_X, y }, LABEL_BOX_SIZE, nsOptionStyle::IDLE_COLOR);

				/* 項目名。 */
				canvas.DrawString(rc, entry.label, { LABEL_TEXT_X, y }, ROW_TEXT_SCALE, textColor, TextAlign::Left);

				/* 値。 */
				wchar_t text[VALUE_TEXT_LENGTH];
				switch (entry.type)
				{
				case OptionRowType::Gauge:
				{
					model.Format(entry, text, VALUE_TEXT_LENGTH);
					canvas.DrawString(rc, text, { GAUGE_NUMBER_X, y }, ROW_TEXT_SCALE, textColor, TextAlign::Right);

					/* ゲージの地と、左から割合ぶんの中身。 */
					canvas.DrawBox({ GAUGE_X, y }, GAUGE_SIZE, nsOptionStyle::GAUGE_BACK_COLOR);
					const float fillWidth = GAUGE_SIZE.x * model.GetRatio(entry);
					if (fillWidth > 0.0f)
					{
						const float fillX = GAUGE_X - GAUGE_SIZE.x * 0.5f + fillWidth * 0.5f;
						canvas.DrawBox({ fillX, y }, { fillWidth, GAUGE_SIZE.y }, nsOptionStyle::ACCENT_COLOR);
					}
					break;
				}

				case OptionRowType::Choice:
					model.Format(entry, text, VALUE_TEXT_LENGTH);
					canvas.DrawString(rc, text, { VALUE_X, y }, ROW_TEXT_SCALE, textColor, TextAlign::Center);
					break;

				default:
					canvas.DrawString(rc, COMMAND_TEXT, { VALUE_X, y }, ROW_TEXT_SCALE, textColor, TextAlign::Center);
					break;
				}

				/* 選んでいる行で、値を動かせるなら左右の矢印。 */
				if (isSelected && entry.type != OptionRowType::Command)
				{
					canvas.DrawString(rc, L"<", { ARROW_LEFT_X, y }, ROW_TEXT_SCALE, textColor, TextAlign::Center);
					canvas.DrawString(rc, L">", { ARROW_RIGHT_X, y }, ROW_TEXT_SCALE, textColor, TextAlign::Center);
				}
			}
		}


		int OptionRowList::HitRow(const Vector2& point, int rowCount, int top)const
		{
			for (int visible = 0; visible < MAX_VISIBLE; visible++)
			{
				const int index = top + visible;
				if (index >= rowCount)
					break;

				if (nsOptionStyle::IsInside(point, { 0.0f, GetRowY(visible) }, BAND_CENTER_X_SIZE))
					return index;
			}
			return -1;
		}


		int OptionRowList::HitArrow(const Vector2& point, int top, int cursor)const
		{
			/* 選んでいる行が見えていなければ矢印も無い。 */
			const int visible = cursor - top;
			if (visible < 0 || visible >= MAX_VISIBLE)
				return 0;

			const float y = GetRowY(visible);

			if (nsOptionStyle::IsInside(point, { ARROW_LEFT_X, y }, ARROW_HIT_SIZE))
				return -1;

			if (nsOptionStyle::IsInside(point, { ARROW_RIGHT_X, y }, ARROW_HIT_SIZE))
				return 1;

			return 0;
		}
	}
}