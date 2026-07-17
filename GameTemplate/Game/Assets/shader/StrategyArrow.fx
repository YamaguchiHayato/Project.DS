// --- 定数バッファ ( register b0 ) ---
cbuffer ConstantBuffer : register(b0)
{
    row_major float4x4 mWorldViewProj;
    float4 mColor; // Alphaに進捗(0.0~1.0)を入れる
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

// --- 構造体定義 (セマンティクスを標準的な形式に修正) ---
struct VS_INPUT
{
    float4 pos : POSITION;
    float4 color : COLOR0; // COLOR -> COLOR0
    float2 uv : TEXCOORD0; // TEXCOORD -> TEXCOORD0
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

// --- 頂点シェーダー ---
PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(input.pos, mWorldViewProj);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

// --- ピクセルシェーダー ---
float4 PSMain(PS_INPUT input) : SV_Target
{
    float4 texColor = g_texture.Sample(g_sampler, input.uv);
    
    // アルファ値から進捗を取得
    float progress = mColor.a;
    
    // 5段階ステップ計算
    float stepProgress = floor(progress * 5.0f + 0.01f) / 5.0f; // 浮動小数誤差対策で+0.01
    float threshold = 1.0f - stepProgress;

    // 境界線より下なら影にする
    if (input.uv.y > threshold)
    {
        texColor.rgb *= 0.2f;
    }
    
    // 最終色決定
    // input.color（頂点カラー）とmColor（SetMulColor）を両方考慮
    float4 finalColor = texColor * input.color * float4(mColor.rgb, 1.0f);
    
    return finalColor;
}