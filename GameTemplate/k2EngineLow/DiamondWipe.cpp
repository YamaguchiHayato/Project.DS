#include "k2EngineLowPreCompile.h" // エンジンのプリコンパイル済みヘッダー
#include "DiamondWipe.h"

using namespace nsK2EngineLow;

void DiamondWipe::Init(const char* filePath, float w, float h)
{
    SpriteInitData initData;

    // 画像パスとサイズ
    initData.m_ddsFilePath[0] = filePath;
    initData.m_width = static_cast<UINT>(w);
    initData.m_height = static_cast<UINT>(h);

    // ★重要: シェーダーをワイプ専用のものにする
    initData.m_fxFilePath = "Assets/shader/DiamondTransition.fx";

    // ★重要: 拡張定数バッファを登録
    // これで自動的に b1 レジスタにデータが送られるようになります
    initData.m_expandConstantBuffer = &m_cbData;
    initData.m_expandConstantBufferSize = sizeof(DiamondConstants);

    // アルファブレンド有効（透過処理のため）
    initData.m_alphaBlendMode = AlphaBlendMode_Trans;

    // Spriteの初期化実行
    m_sprite.Init(initData);
}

void DiamondWipe::Draw(RenderContext& rc)
{
    // 座標等の更新
    m_sprite.Update(m_position, m_rotation, m_scale, m_pivot);

    // レンダリングエンジンに「自分を描画してくれ」と登録
    // これにより、適切なタイミングで OnRender2D が呼ばれます
    g_renderingEngine->AddRenderObject(this);
}