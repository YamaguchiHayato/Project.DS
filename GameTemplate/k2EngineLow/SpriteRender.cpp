#include "k2EngineLowPreCompile.h"
#include "SpriteRender.h"

using namespace nsK2EngineLow;

void nsK2EngineLow::SpriteRender::Init(const char* filePath, const float w, const float h, AlphaBlendMode alphaBlendMode)
{
	SpriteInitData initData;
	//DDSファイルパスを格納する変数の設定。
	initData.m_ddsFilePath[0] = filePath;//ここいじらないとSpriteでPng使えないよ！分岐させよっかな！
	//Spriteで使用するシェーダーパスを設定。
	initData.m_fxFilePath = "Assets/shader/sprite.fx";
	//スプライトの縦横幅を設定。
	initData.m_width = w;
	initData.m_height = h;
	initData.m_alphaBlendMode = alphaBlendMode;
	//SpriteRenderノ初期データを使用してSpritereaderを初期化。
	m_sprite.Init(initData);
}

void nsK2EngineLow::SpriteRender::Init(const char* filePath, const float w, const float h,const char* fxPath, AlphaBlendMode alphaBlendMode)
{
	SpriteInitData initData;
	//DDSファイルパスを格納する変数の設定。
	initData.m_ddsFilePath[0] = filePath;//ここいじらないとSpriteでPng使えないよ！分岐させよっかな！
	//Spriteで使用するシェーダーパスを設定。
	initData.m_fxFilePath = fxPath;
	//スプライトの縦横幅を設定。
	initData.m_width = w;
	initData.m_height = h;
	initData.m_alphaBlendMode = alphaBlendMode;
	//SpriteRenderノ初期データを使用してSpritereaderを初期化。
	m_sprite.Init(initData);
}

void nsK2EngineLow::SpriteRender::Draw(RenderContext& rc)
{
	g_renderingEngine->AddRenderObject(this);

}


