#include "stdafx.h"
#include "BackgroundSprite.h"

namespace
{
	const char* sSpriteShaderPath_ = "Assets/shader/sprite.fx";	//! 画像を描くシェーダー。SpriteRender と同じものを使う。
}

namespace nsApp
{
	namespace nsUI
	{
		void BackgroundSprite::Init(const char* pFilePath, float fWidth, float fHeight)
		{
			SpriteInitData stInitData;
			stInitData.m_ddsFilePath[0] = pFilePath;
			stInitData.m_fxFilePath = sSpriteShaderPath_;
			stInitData.m_width = static_cast<UINT>(fWidth);
			stInitData.m_height = static_cast<UINT>(fHeight);
			stInitData.m_alphaBlendMode = AlphaBlendMode_Trans;

			/*
			 * 描き込む先はメインのレンダリングターゲット(浮動小数点の形式)。
			 * スプライトの既定は 2D 用の8bitの形式なので、そのままだと形式が合わずに描けない。
			 */
			stInitData.m_colorBufferFormat[0] = g_renderingEngine->GetMainRenderTarget().GetColorBufferFormat();
			stSprite_.Init(stInitData);

			/* 画面の中央に置く。動かさないので、位置はここで一度だけ決める。*/
			stSprite_.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
		}


		void BackgroundSprite::SetMulColor(const Vector4& vMulColor)
		{
			/* 画像に掛ける色を入れる。*/
			stSprite_.SetMulColor(vMulColor);
		}


		void BackgroundSprite::Draw(RenderContext& rc)
		{
			/* 描くのは OnRenderBackground2D の中。ここでは登録だけする。*/
			g_renderingEngine->AddRenderObject(this);
		}


		void BackgroundSprite::OnRenderBackground2D(RenderContext& rc)
		{
			/* モデルより先に描く。深度は書かないので、あとのモデルが必ず上に乗る。*/
			stSprite_.Draw(rc);
		}
	}
}
