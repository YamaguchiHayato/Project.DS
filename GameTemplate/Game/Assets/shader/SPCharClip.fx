// --- 定数バッファ ---
cbuffer HPGaugeCB : register(b1)
{
    float hpRate;
    float startAngle;
    float maxAngle;
    float innerRadius;
    float outerRadius;
    float damageFlash;
    float2 centerPos; // お皿の中心座標
};

// スプライトの行列（エンジンが自動で渡すもの）
cbuffer SpriteCB : register(b0)
{
    float4x4 matWorld;
};

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

// --- 頂点シェーダー (VSMain) ---
// これがないと描画が始まりません
PSInput VSMain(VSInput input)
{
    PSInput output;
    // 頂点座標に行列をかけて、画面上の正しい位置に飛ばします
    output.pos = mul(matWorld, input.pos);
    output.uv = input.uv;
    return output;
}

// --- ピクセルシェーダー (PSMain) ---
float4 PSMain(PSInput input) : SV_Target
{
    float4 texColor = g_texture.Sample(g_sampler, input.uv);

    texColor.rgb += 0.1f; // 少し明るくする
    // ★テスト用：お皿の中心座標を直接指定（1600x900想定ならこのあたり）
    // ズレていたらこの数値をいじってください
    float2 testCenter = float2(800.0, 850.0);
    
    // 現在のピクセルがお皿の中心からどれくらい離れているか
    float2 d = input.pos.xy - testCenter;
    float dist = length(d);
    float angle = atan2(-d.y, d.x);

    // ★テスト用：一旦、角度による切り抜きを無効化
    // if (angle < startAngle || angle > maxAngle) discard;

    // ★半径による切り抜き：C++側の 0.4 ではなく、ピクセル単位（400等）で判定する
    // 一旦 1000.0 にすれば、ほぼ消えずに表示されるはずです
    if (dist > 150.0)
        discard;


    if (texColor.a <= 0.0)
        discard;

    return texColor;
}