#include "k2EngineLowPreCompile.h"
#include "Bloom.h"

nsK2EngineLow::Bloom::Bloom()
{
}

nsK2EngineLow::Bloom::~Bloom()
{
}

void nsK2EngineLow::Bloom::Init(RenderTarget& renderTarget)
{
	//輝度抽出用スプライト用レンダーターゲットの作成。
	luminanceRenderTarget.Create(
		renderTarget.GetWidth(),
		renderTarget.GetHeight(),
		1,
		1,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_D32_FLOAT
	);
	
	//輝度抽出スプライトの初期化。
	luminanceSpriteInitData.m_fxFilePath = "Assets/shader/PostEffect.fx";
	luminanceSpriteInitData.m_vsEntryPointFunc = "VSMain";
	luminanceSpriteInitData.m_psEntryPoinFunc = "PSSamplingLuminance";
	luminanceSpriteInitData.m_width = renderTarget.GetWidth();
	luminanceSpriteInitData.m_height = renderTarget.GetHeight();
	luminanceSpriteInitData.m_textures[0] = &renderTarget.GetRenderTargetTexture();
	luminanceSpriteInitData.m_colorBufferFormat[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	m_luminanceSprite.Init(luminanceSpriteInitData);



	//ガウシアンブラー。
	gaussianBulur[0].Init(&luminanceRenderTarget.GetRenderTargetTexture());
	gaussianBulur[1].Init(&gaussianBulur[0].GetBokeTexture());
	gaussianBulur[2].Init(&gaussianBulur[1].GetBokeTexture());
	gaussianBulur[3].Init(&gaussianBulur[2].GetBokeTexture());


	finalSpriteInitData.m_textures[0] = &gaussianBulur[0].GetBokeTexture();
	finalSpriteInitData.m_textures[1] = &gaussianBulur[1].GetBokeTexture();
	finalSpriteInitData.m_textures[2] = &gaussianBulur[2].GetBokeTexture();
	finalSpriteInitData.m_textures[3] = &gaussianBulur[3].GetBokeTexture();
	finalSpriteInitData.m_width = renderTarget.GetWidth();
	finalSpriteInitData.m_height = renderTarget.GetHeight();
	//ボケ画像を合成する必要があるので２D用シェーダーではなく専用シェーダーを利用する。
	finalSpriteInitData.m_fxFilePath = "Assets/shader/PostEffect.fx";
	finalSpriteInitData.m_psEntryPoinFunc = "PSBloomFinal";
	//ただし加算合成で描画するのでアルファブレンディングモードを加算する。
	finalSpriteInitData.m_alphaBlendMode = AlphaBlendMode_Add;
	finalSpriteInitData.m_colorBufferFormat[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	m_finalSprite.Init(finalSpriteInitData);
	
}

void nsK2EngineLow::Bloom::Update()
{
}

void nsK2EngineLow::Bloom::Render(RenderContext& rc, RenderTarget&renderTarget)
{
	//輝度抽出用ターゲットに変更して使えるようになるまで待つ。
	rc.WaitUntilToPossibleSetRenderTarget(luminanceRenderTarget);
	//ターゲットセット。
	rc.SetRenderTargetAndViewport(luminanceRenderTarget);
	//ターゲットのクリア。
	rc.ClearRenderTargetView(luminanceRenderTarget);
	//描画し終わるまで待つ。
	m_luminanceSprite.Draw(rc);
	
	rc.WaitUntilFinishDrawingToRenderTarget(luminanceRenderTarget);
	//luminanceRenderTarget終了。

	//ガウシアンブラーの実行。
	gaussianBulur[0].ExecuteOnGPU(rc, 20);
	gaussianBulur[1].ExecuteOnGPU(rc, 20);
	gaussianBulur[2].ExecuteOnGPU(rc, 20);
	gaussianBulur[3].ExecuteOnGPU(rc, 20);

	//mainRenderTargetのセット。
	rc.WaitUntilToPossibleSetRenderTarget(renderTarget);
	rc.SetRenderTargetAndViewport(renderTarget);
	//ボケ画像をメインレンダリングターゲットに加算合成（mainRenderTargetトブルームを合体させる）。
	m_finalSprite.Draw(rc);
	//終了待ち。
	rc.WaitUntilFinishDrawingToRenderTarget(renderTarget);
}
