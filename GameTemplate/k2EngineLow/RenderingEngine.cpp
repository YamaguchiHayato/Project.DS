#include "k2EngineLowPreCompile.h"
#include "RenderingEngine.h"
#include <algorithm>
#include <execution> // PMXのCPUアニメーション並列更新用


void nsK2EngineLow::RenderingEngine::Init()
{
	InitMainRenderTarget();

	//2D(フォントとスプライト)の初期化。
	Init2DSprite();

	//メインレンダリングターゲットのカラーバッファの内容を
	//フレームバッファにコピーするためのスプライトを初期化する
	InitFinalSprite();

	// ★追加: モーションブラー用スプライトの初期化
	// ※InitMainRenderTargetの後ならここでもOKです
	InitMotionBlurSprite();

	//ブルームの初期化。
	m_bloom.Init(m_mainRenderingTarget);

	//シャドウマップの初期化。
	m_shadow.Init();

	//ライトの初期化。
	m_light.Init();
}
void nsK2EngineLow::RenderingEngine::InitMainRenderTarget()
{
	//メインレンダリングターゲットの作成
	float clearColor[4] = { 0.5f,0.5f,0.5f,1.0f };
	m_mainRenderingTarget.Create(
		g_graphicsEngine->GetFrameBufferWidth(),
		g_graphicsEngine->GetFrameBufferHeight(),
		1,
		1,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_D32_FLOAT,
		clearColor
	);

	   // ★追加: ベロシティバッファの作成
		   // R16G16_FLOAT (2成分の浮動小数点) で作成
		float clearColorVel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_velocityRenderTarget.Create(
		g_graphicsEngine->GetFrameBufferWidth(),
		g_graphicsEngine->GetFrameBufferHeight(),
		1,
		1,
		DXGI_FORMAT_R16G16B16A16_FLOAT, // ★重要
		DXGI_FORMAT_UNKNOWN,      // 深度バッファはメインと共有するので不要
		clearColorVel
	);

	//デバッグ用出力。
	/*char buf[128];
	sprintf_s(buf, "MainRT ColorBufferFormat = 0x%x\n", m_mainRenderingTarget.GetColorBufferFormat());
	OutputDebugStringA(buf);*/
}

void nsK2EngineLow::RenderingEngine::Init2DSprite()
{
	float clearColor[4] = { 0.0f,0.0f,0.0f,0.0f };

	//2D用のターゲットの初期化。
	m_2DRenderTarget.Create(
		m_mainRenderingTarget.GetWidth(),
		m_mainRenderingTarget.GetHeight(),
		1,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_UNKNOWN,
		clearColor
	);

	//最終合成用のスプライトを初期化する。
	SpriteInitData spriteInitData;
	//テクスチャは2Dレンダーターゲット。
	spriteInitData.m_textures[0] = &m_2DRenderTarget.GetRenderTargetTexture();
	// 解像度はmainRenderTargetの幅と高さ
	spriteInitData.m_width = m_mainRenderingTarget.GetWidth();
	spriteInitData.m_height = m_mainRenderingTarget.GetHeight();
	// 2D用のシェーダーを使用する
	spriteInitData.m_fxFilePath = "Assets/shader/sprite.fx";
	spriteInitData.m_vsEntryPointFunc = "VSMain";
	spriteInitData.m_psEntryPoinFunc = "PSMain";
	//上書き。
	spriteInitData.m_alphaBlendMode = AlphaBlendMode_None;
	//レンダリングターゲットのフォーマット。
	spriteInitData.m_colorBufferFormat[0] = m_mainRenderingTarget.GetColorBufferFormat();

	m_2DSprite.Init(spriteInitData);

	//テクスチャはメインレンダ―ターゲット。
	spriteInitData.m_textures[0] = &m_mainRenderingTarget.GetRenderTargetTexture();

	//解像度は2Dレンダ―ターゲットの幅と高さ
	spriteInitData.m_width = m_2DRenderTarget.GetWidth();
	spriteInitData.m_height = m_2DRenderTarget.GetHeight();
	spriteInitData.m_fxFilePath = "Assets/shader/sprite.fx";
	spriteInitData.m_alphaBlendMode = AlphaBlendMode_None;

	//レンダリングターゲットのフォーマット。
	spriteInitData.m_colorBufferFormat[0] = m_2DRenderTarget.GetColorBufferFormat();

	m_mainSprite.Init(spriteInitData);
}

//メインレンダリングターゲットのカラーバッファの内容を
	//フレームバッファにコピーするためのスプライトを初期化する
void nsK2EngineLow::RenderingEngine::InitFinalSprite()
{
	SpriteInitData spriteInitData;
	// ▼▼▼ ベロシティのデバッグ変更ここから ▼▼▼
	// 本来のメイン画像の代わりに、ベロシティバッファを表示するようにセットする
	spriteInitData.m_textures[0] = &m_mainRenderingTarget.GetRenderTargetTexture();//本来のメイン画像
	//spriteInitData.m_textures[0] = &m_velocityRenderTarget.GetRenderTargetTexture();//ベロシティバッファ
	// ▲▲▲ デバッグ変更ここまで ▲▲▲
	spriteInitData.m_width = m_mainRenderingTarget.GetWidth();
	spriteInitData.m_height = m_mainRenderingTarget.GetHeight();
	spriteInitData.m_fxFilePath = "Assets/shader/sprite.fx";
	m_copyToframeBufferSprite.Init(spriteInitData);
}

void nsK2EngineLow::RenderingEngine::InitMotionBlurSprite()
{
	// 1. 作業用バッファ (Mainと同じ設定)
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_tempRenderTarget.Create(
		g_graphicsEngine->GetFrameBufferWidth(),
		g_graphicsEngine->GetFrameBufferHeight(),
		1, 1,
		DXGI_FORMAT_R32G32B32A32_FLOAT, // Mainと同じ高精度フォーマット推奨
		DXGI_FORMAT_UNKNOWN,
		clearColor
	);

	// 2. モーションブラー用スプライト
	SpriteInitData blurData;
	blurData.m_width = g_graphicsEngine->GetFrameBufferWidth();
	blurData.m_height = g_graphicsEngine->GetFrameBufferHeight();
	blurData.m_fxFilePath = "Assets/shader/MotionBlur.fx"; // さっき作ったシェーダー
	blurData.m_vsEntryPointFunc = "VSMain";
	blurData.m_psEntryPoinFunc = "PSMain";
	// テクスチャ0: メイン画像 (色)
	blurData.m_textures[0] = &m_mainRenderingTarget.GetRenderTargetTexture();
	// テクスチャ1: 速度画像 (ベロシティ)
	blurData.m_textures[1] = &m_velocityRenderTarget.GetRenderTargetTexture(); // ★ここがポイント
	blurData.m_colorBufferFormat[0] = m_tempRenderTarget.GetColorBufferFormat(); // 書き込み先はTemp
	m_motionBlurSprite.Init(blurData);

	// 3. 書き戻し用スプライト (Temp -> Main)
	SpriteInitData copyData;
	copyData.m_width = blurData.m_width;
	copyData.m_height = blurData.m_height;
	copyData.m_fxFilePath = "Assets/shader/sprite.fx"; // 普通のコピーでOK
	copyData.m_textures[0] = &m_tempRenderTarget.GetRenderTargetTexture(); // Tempを元に
	copyData.m_colorBufferFormat[0] = m_mainRenderingTarget.GetColorBufferFormat(); // Mainに戻す
	m_copySprite.Init(copyData);
}

void nsK2EngineLow::RenderingEngine::Execute(RenderContext& rc)
{
	//ライトカメラの更新。
	m_light.LightCameraUpdate();

	////影。
	m_shadow.Execute(rc, m_renderObjects);

	if (isResultFlag == false) {
		//モデルの描画。
		ModelDraw(rc);
	}

	//// === PMXモデル描画 ===
	PMXModelDraw(rc);

	// === ペンライト（観客）描画 === PMXの直後・ブルーム前に描くと発光がブルームに乗る
	if (m_penlight) {
		m_penlight->Draw(rc);
	}

	//エフェクトの描画。
	EffectEngine::GetInstance()->Draw();


	//ブルームの適用。
	m_bloom.Render(rc, m_mainRenderingTarget);

	// ===========================================================
	// ★ モーションブラー処理開始（isMotionBlurEnabled==false のとき丸ごとスキップ）
	// ===========================================================
	if (isMotionBlurEnabled) {
		// 1. Tempバッファをターゲットに設定
		rc.WaitUntilToPossibleSetRenderTarget(m_tempRenderTarget);
		rc.SetRenderTargetAndViewport(m_tempRenderTarget);
		rc.ClearRenderTargetView(m_tempRenderTarget); // 念のためクリア

		// ★ここでパラメータを指定！
		// R = 0.5f  --> 強さ 10.0 (0.5 * 20)
		// G = 0.4f  --> 回数 8回  (0.4 * 20)
		// B = 0.0f  --> 未使用
		// A = 1.0f  --> 未使用(シェーダーで無視)
		m_motionBlurSprite.SetMulColor({ 0.001f, 0.8f, 0.0f, 1.0f });

		// 2. ブラーを描画 (Main + Vel -> Temp)
		//    MainRTとVelRTは読み込みに使うので、書き込み待ちなどは不要(RenderTarget設定してないので)
		//    ただしリソースバリアの遷移が必要な場合があるが、Spriteクラスがやってくれると信じる
		m_motionBlurSprite.Draw(rc);

		rc.WaitUntilFinishDrawingToRenderTarget(m_tempRenderTarget);


		// 3. 書き戻し (Temp -> Main)
		rc.WaitUntilToPossibleSetRenderTarget(m_mainRenderingTarget);
		rc.SetRenderTargetAndViewport(m_mainRenderingTarget);

		m_copySprite.Draw(rc);

		rc.WaitUntilFinishDrawingToRenderTarget(m_mainRenderingTarget);
	}
	// ===========================================================
	// ★ モーションブラー処理終了
	// ===========================================================


	//画像と文字の描画。
	SpriteFontDraw(rc);
	//リザルト画面なうなら。
	if (isResultFlag == true) {
		ModelDraw(rc);
	}
	//メインレンダリングターゲットの絵をフレームバッファにコピー。
	CopyMainRenderTargetToFrameBuffer(rc);

	//クリア。
	m_renderObjects.clear();

	//PMXモデル描画用配列のクリア。
	// ★ PMX は通常“常駐”が多いのでクリアしない（必要に応じて管理側で消す）
	//m_pmxObjects.clear();
}

void nsK2EngineLow::RenderingEngine::ModelDraw(RenderContext& rc)
{
	// メインのターゲットが使えるようになるまで待つ
	rc.WaitUntilToPossibleSetRenderTarget(m_mainRenderingTarget);
	// ターゲットセット
	rc.SetRenderTargetAndViewport(m_mainRenderingTarget);
	// ターゲットのクリア
	rc.ClearRenderTargetView(m_mainRenderingTarget);

	// まとめてモデルレンダーを描画
	for (auto MobjData : m_renderObjects) {
		MobjData->OnRenderModel(rc);
	}

	// 描画されるまで待つ
	rc.WaitUntilFinishDrawingToRenderTarget(m_mainRenderingTarget);
}

void nsK2EngineLow::RenderingEngine::SpriteFontDraw(RenderContext& rc)
{
	//2D用のターゲットが使えるようになるまで待つ。
	rc.WaitUntilToPossibleSetRenderTarget(m_2DRenderTarget);
	//ターゲットのセット。
	rc.SetRenderTargetAndViewport(m_2DRenderTarget);
	//ターゲットのクリア。
	rc.ClearRenderTargetView(m_2DRenderTarget);

	m_mainSprite.Draw(rc);

	//スプライトと文字の描画。
	for (auto SobjData : m_renderObjects) {
		SobjData->OnRender2D(rc);
	}

	// 描画されるまで待つ
	rc.WaitUntilFinishDrawingToRenderTarget(m_2DRenderTarget);
	
	//ターゲットをメインに戻す
	rc.WaitUntilToPossibleSetRenderTarget(m_mainRenderingTarget);
	// レンダリングターゲットをセット
	rc.SetRenderTargetAndViewport(m_mainRenderingTarget);
	//2Dの描画。
	m_2DSprite.Draw(rc);
	// mainRenderingTargetの描画が終わるまで待つ
	rc.WaitUntilFinishDrawingToRenderTarget(m_mainRenderingTarget);
}

void nsK2EngineLow::RenderingEngine::CopyMainRenderTargetToFrameBuffer(RenderContext& rc)
{
	//フレームバッファにセット。
	rc.SetRenderTarget(
		g_graphicsEngine->GetCurrentFrameBuffuerRTV(),
		g_graphicsEngine->GetCurrentFrameBuffuerDSV()
	);
	//ビューポートとシザリング短形の設定
	rc.SetViewportAndScissor(g_graphicsEngine->GetFrameBufferViewport());
	
	m_copyToframeBufferSprite.Draw(rc);
}

void nsK2EngineLow::RenderingEngine::PMXModelDraw(RenderContext& rc)
{

	////OutputDebugStringA("PMXModelDraw: begin\n");

	//if (m_pmxObjects.empty()) return;

	//// メインRTへ
	//rc.WaitUntilToPossibleSetRenderTarget(m_mainRenderingTarget);
	//rc.SetRenderTargetAndViewport(m_mainRenderingTarget);
	//// ※ ここでクリアはしない（ModelDrawで済ませているため）
	////    PMXを完全に独立パスにしたいならここで Clear してもOK

	//// PMX を順に描画（PMXRender 内で RootSig/PSO/CBV/SRV をセットする前提）
	//for (auto* pmx : m_pmxObjects) {
	//	if (!pmx) {
	//		continue;
	//	}
	//	//OutputDebugStringA("PMXModelDraw: draw one PMX\n");
	//	pmx->Draw(rc);
	//}

	//rc.WaitUntilFinishDrawingToRenderTarget(m_mainRenderingTarget);
	////OutputDebugStringA("PMXModelDraw: end\n");

	// ★修正後: MRT対応
	if (m_pmxObjects.empty()) return;

	// =============================================================
	// ★CPUアニメーション更新を全モデル【並列】で実行する。
	//   UpdateAnimation はGPUに一切触らない設計（VMD再生・ボーン・IK・物理・
	//   モーフ計算のみ。GPU転送は各 Draw の冒頭でシリアルに行う）なので、
	//   モデルごとにワーカースレッドへ分散できる。
	//   9体で最重量級だったボーン更新＋物理が並列化され、モデル数が増えても
	//   コア数でスケールするためFPSが落ちにくい。
	//   【前提】PMXModel / VMDLoader を PMXRender 間で共有しないこと。
	// =============================================================
	std::for_each(std::execution::par, m_pmxObjects.begin(), m_pmxObjects.end(),
		[](PMXRender* p) {
			if (p) p->UpdateAnimation();
		});

	// 1. 書き込み先リスト (メイン + 速度)
		RenderTarget * rts[2] = {
		&m_mainRenderingTarget,
		&m_velocityRenderTarget
		 };
	
	// 2. 書き込み待ち
	rc.WaitUntilToPossibleSetRenderTargets(2, rts);
	
	// 3. 速度バッファのクリア (黒=移動なし)
	// メインRTは直前のModelDrawでクリア済みなので、ここでは速度だけクリア
	rc.ClearRenderTargetView(
		m_velocityRenderTarget.GetRTVCpuDescriptorHandle(),
		m_velocityRenderTarget.GetRTVClearColor()
	 );
	
	// 4. レンダーターゲットセット (MRT)
	// メインRTの深度バッファ(DSV)を共有して使う
	rc.SetRenderTargets(2, rts, m_mainRenderingTarget.GetDSVCpuDescriptorHandle());
	
	// 5. ビューポート設定
	// RenderTargetから幅と高さを取得して、ビューポートを作成する
	D3D12_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(m_mainRenderingTarget.GetWidth());
	viewport.Height = static_cast<float>(m_mainRenderingTarget.GetHeight());
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = D3D12_MIN_DEPTH;
	viewport.MaxDepth = D3D12_MAX_DEPTH;

	rc.SetViewportAndScissor(viewport);
	
	// 6. 描画ループ
	for (auto* pmx : m_pmxObjects) {
		if (!pmx) continue;
		pmx->Draw(rc);
		
	}
	
	// 7. 完了待ち
	rc.WaitUntilFinishDrawingToRenderTargets(2, rts);
}

void nsK2EngineLow::RenderingEngine::RemovePMXObject(PMXRender* obj) {
	auto it = std::find(m_pmxObjects.begin(), m_pmxObjects.end(), obj);
	if (it != m_pmxObjects.end()) m_pmxObjects.erase(it);
}