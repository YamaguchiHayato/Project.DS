#include "k2EngineLowPreCompile.h" // エンジンのプリコンパイル済みヘッダー
#include "UIGaugeArc.h"

using namespace nsK2EngineLow;

void  UIGaugeArc::Init(const char* filePath, float w, float h, const char* fxPath) {
    SpriteInitData initData;

    // 画像パスとサイズ
    initData.m_ddsFilePath[0] = filePath;
    initData.m_width = static_cast<UINT>(w);
    initData.m_height = static_cast<UINT>(h);

    // ★重要: シェーダーをワイプ専用のものにする
    initData.m_fxFilePath = fxPath;

    // ★重要: 拡張定数バッファを登録
    // これで自動的に b1 レジスタにデータが送られるようになります
    initData.m_expandConstantBuffer = &m_hpGaugeCBData;
    initData.m_expandConstantBufferSize = sizeof(HPGaugeCB);

    // アルファブレンド有効（透過処理のため）
    initData.m_alphaBlendMode = AlphaBlendMode_Trans;

    // Spriteの初期化実行
    m_sprite.Init(initData);
}

// 引数に角度や半径を追加して、外から制御できるようにする
void UIGaugeArc::Update(float hpRate, float damageFlash, float startAngle, float maxAngle, float innerRadius, float outerRadius) {
    // 構造体のデータを更新
    m_hpGaugeCBData.hpRate = hpRate;
    m_hpGaugeCBData.damageFlash = damageFlash;

    // 引数で受け取った値を使う
    m_hpGaugeCBData.startAngle = startAngle;
    m_hpGaugeCBData.maxAngle = maxAngle;
    m_hpGaugeCBData.innerRadius = innerRadius;
    m_hpGaugeCBData.outerRadius = outerRadius;

    // 座標等の更新
    m_sprite.Update(m_position, m_rotation, m_scale, m_pivot);
}

void UIGaugeArc::Draw(RenderContext& rc) {
    // 座標等の更新
    m_sprite.Update(m_position, m_rotation, m_scale, m_pivot);

    // レンダリングエンジンに「自分を描画してくれ」と登録
    // これにより、適切なタイミングで OnRender2D が呼ばれます
    g_renderingEngine->AddRenderObject(this);
}
