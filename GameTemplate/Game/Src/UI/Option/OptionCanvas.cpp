#include "stdafx.h"
#include "OptionCanvas.h"

namespace
{
	const char* FRAME_PATH = "Assets/image/config/configFrame.dds"; //! 枠の画像。差し替えはここだけ。
	const char* SPRITE_FX_PATH = "Assets/shader/sprite.fx"; //! 枠のシェーダー。
	const float BASE_SIZE = 100.0f; //! 枠の元の大きさ。描くときはこれに対する倍率で広げる。
	const Vector2 PIVOT_CENTER = { 0.5f, 0.5f }; //! 枠は中心基準。
	const Vector2 TEXT_PIVOT = { 0.0f, 1.0f }; //! 文字は左上基準。
}


namespace nsApp
{
	namespace nsUI
	{
		bool OptionCanvas::Start()
		{
			/* 枠の画像を1枚だけ読む。 */
			wchar_t widePath[260];
			size_t converted = 0;
			mbstowcs_s(&converted, widePath, FRAME_PATH, 259);
			texture_.InitFromDDSFile(widePath);

			/* 全部の枠に同じテクスチャを渡す。 */
			SpriteInitData data;
			data.m_textures[0] = &texture_;
			data.m_fxFilePath = SPRITE_FX_PATH;
			data.m_width = static_cast<UINT>(BASE_SIZE);
			data.m_height = static_cast<UINT>(BASE_SIZE);
			data.m_alphaBlendMode = AlphaBlendMode_Trans;
			for (int index = 0; index < MAX_BOX; index++)
				box_[index].Init(data);

			/* 文字は左上基準にそろえる。揃えは DrawString で計算する。 */
			for (int index = 0; index < MAX_TEXT; index++)
				text_[index].SetPivot(TEXT_PIVOT);

			return true;
		}


		void OptionCanvas::Begin()
		{
			/* 割り当てを先頭に戻す。 */
			boxCount_ = 0;
			textCount_ = 0;

			/* 文字より先に登録し、枠が文字の下になるようにする。 */
			g_renderingEngine->AddRenderObject(this);
		}


		void OptionCanvas::DrawBox(const Vector2& center, const Vector2& size, const Vector4& color)
		{
			/* 用意した数を超えたら描かない。 */
			if (boxCount_ >= MAX_BOX)
				return;
				
			Sprite& sprite = box_[boxCount_];
			boxCount_++;

			sprite.SetMulColor(color);
			sprite.Update({ center.x, center.y, 0.0f },Quaternion::Identity,{ size.x / BASE_SIZE, size.y / BASE_SIZE, 1.0f },PIVOT_CENTER);
		}


		void OptionCanvas::DrawString(RenderContext& rc, const wchar_t* text, const Vector2& position, float scale, const Vector4& color, TextAlign align)
		{
			/* 用意した数を超えたら描かない。 */
			if (textCount_ >= MAX_TEXT)
				return;

			FontRender& font = text_[textCount_];
			textCount_++;

			font.SetText(text);
			font.SetScale(scale);
			font.SetColor(color);

			/* 実際の大きさを測り、揃えに合わせて左上の位置を決める。 */
			const Vector2 size = font.MeasureText();
			float x = position.x;

			/* 横の揃えに合わせて左上の位置を決める。 */
			if (align == TextAlign::Center)
				x -= size.x * 0.5f;
			else if (align == TextAlign::Right)
				x -= size.x;

			/* 文字は左上から下へ伸びるので、上端 = 中心 + 高さの半分。 */
			font.SetPosition({ x, position.y + size.y * 0.5f, 0.0f });
			font.Draw(rc);
		}


		void OptionCanvas::OnRender2D(RenderContext& rc)
		{
			/* このフレームで割り当てた枠だけ描く。 */
			for (int index = 0; index < boxCount_; index++)
				box_[index].Draw(rc);
		}
	}
}