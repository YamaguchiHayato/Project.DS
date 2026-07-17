// --- 定数バッファ ---

// b0: エンジン標準 (Spriteクラスが自動転送)
cbuffer LocalConstantBuffer : register(b0)
{
    float4x4 mvp; // 行列
    float4 mulColor; // 乗算色
    float4 screenParam; // スクリーンパラメータ
};

// b1: ユーザー拡張 (UIGaugeArc::HPGaugeCB と一致)
cbuffer HPGaugeCB : register(b1)
{
    float hpRate; // HP割合 (0.0～1.0)
    float startAngle; // 開始角度 (左端: PI)
    float maxAngle; // 終了角度 (右端: 0.0)
    float innerRadius; // 内径 (0.0～1.0)
    float outerRadius; // 外径 (0.0～1.0)
    float damageFlash; // ダメージ時の光り具合
    float2 padding; // 16バイト境界合わせ
};

// --- テクスチャ ---
Texture2D<float4> g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct VSInput
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// --- 頂点シェーダー ---
PSInput VSMain(VSInput input)
{
    PSInput output;
    // エンジンのMVP行列を使用して、ポリゴンを正しい位置・サイズに変換
    output.pos = mul(mvp, input.pos);
    output.uv = input.uv;
    return output;
}

// ピクセルシェーダー部分
float4 PSMain(PSInput input) : SV_Target
{
    float2 center = float2(0.5, 0.5);
    float2 pos = input.uv - center;
    float dist = length(pos);
    float angle = atan2(-pos.y, pos.x); // 右上は 0.0 ～ 1.57

    // 1. 右上の範囲外をカット
    if (angle < startAngle || angle > maxAngle)
        discard;

    // 2. ボルテージによる切り抜き
    // hpRate 1.0 (満タン) のとき、境界は 1.57 (真上までOK)
    // hpRate 0.0 (空っぽ) のとき、境界は 0.0 (真右の開始地点)
    float currentLimitAngle = lerp(startAngle, maxAngle, hpRate);

    // 【重要】現在のピクセル角度が limit より「大きい（上に近い）」なら捨てる
    // これにより、0.0(右)から徐々に上へゲージが伸びていきます
    if (angle > currentLimitAngle)
    {
        discard;
    }

    // 3. 半径カット
    if (dist < innerRadius || dist > outerRadius)
        discard;

    // 4. カラー出力
    float4 texColor = g_texture.Sample(g_sampler, input.uv);
    float3 voltageColor = float3(1.0, 0.6, 0.0); // オレンジ
    
    float3 finalColor = voltageColor * texColor.rgb;

    if (damageFlash < 0.0f)
    {
    // 背景モード：色を明るいグレーにする
        finalColor = (0.8f,0.8f,0.8f);
    }
    else
    {
        //通常のボルテージゲージ：ダメージフラッシュを適用
        finalColor = lerp(finalColor, float3(1, 1, 1), damageFlash);
    }
    return float4(finalColor, texColor.a);
}