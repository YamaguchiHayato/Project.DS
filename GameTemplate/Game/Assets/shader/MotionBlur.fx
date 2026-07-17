// ==============================================
// MotionBlur.fx (RGBパラメータ版)
// ==============================================

// --- 定数バッファ ---
cbuffer cb : register(b0)
{
    float4x4 g_mvp; // 行列
    float4 g_color; // ★これを使います！(SetMulColorの値)
};

// cbParam (b1) はもう使わないので削除してOK

// --- テクスチャ ---
Texture2D g_texColor : register(t0); // MainRT
Texture2D g_texVelocity : register(t1); // VelocityRT
SamplerState g_sampler : register(s0);

// --- 頂点シェーダー ---
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    output.pos = mul(float4(input.pos, 1.0f), g_mvp);
    output.uv = input.uv;
    return output;
}

// --- ピクセルシェーダー ---
float4 PSMain(VS_OUT input) : SV_TARGET
{
    // =========================================================
    // ★パラメータの受け取り (RGBハック)
    // =========================================================
    // C++側で SetMulColor(R, G, B, A) した値を使います。
    // R (0.0~1.0) -> ブラー強度 (0.0 ~ 20.0 に変換)
    // G (0.0~1.0) -> サンプリング回数 (0 ~ 20回 に変換)
    
    float strength = g_color.r * 20.0f; // R成分を強さに
    int numSamples = (int) (g_color.g * 20.0f); // G成分を回数に

    // 安全対策：回数が0以下なら1回(ブラーなし)にする
    if (numSamples <= 1)
        numSamples = 1;

    // =========================================================

    // 1. 速度とマスクを取得 (.w がマスク)
    float4 velocitySample = g_texVelocity.Sample(g_sampler, input.uv);
    float2 velocity = velocitySample.xy;
    float mask = velocitySample.w;
    velocity *= strength;
    
    // ★追加: マスク適用
    // maskが0(背景やステージ)なら、velocityが0になりブラーが無効化される
    velocity *= mask;
    
    // ★★★ ここに追加：安全装置 (ベロシティ・クランプ) ★★★
    // 移動量が大きすぎると画面外(黒)を拾ってノイズになるため、長さを制限します。
    // 0.1f とは「画面幅の10%」という意味です。これ以上長いブラーは制限します。
    float maxBlurLength = 0.05f;
    float velLen = length(velocity);
    
    if (velLen > maxBlurLength)
    {
        // 方向は変えずに、長さだけ maxBlurLength に縮める
        velocity = (velocity / velLen) * maxBlurLength;
    }
    // ★★★ 追加ここまで ★★★

    // 2. メイン画像の色を取得
    float4 color = g_texColor.Sample(g_sampler, input.uv);

    // 3. ブラー回転 (サンプリング)
    for (int i = 1; i < numSamples; ++i)
    {
        float2 offset = velocity * (float(i) / float(numSamples - 1));
        color += g_texColor.Sample(g_sampler, input.uv + offset);
    }

    // 平均化
    color /= float(numSamples);

    // ★重要: スプライトの元々の色(g_color)を乗算してしまうと
    // パラメータのせいで画面が暗くなったり赤くなったりするので、
    // RGBは無視して、Alphaだけ1.0(不透明)にして出力します。
    return float4(color.rgb, 1.0f);
}