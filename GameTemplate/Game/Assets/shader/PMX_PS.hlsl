Texture2D g_albedo : register(t0);
SamplerState g_samp : register(s0);

// ★VS と同じ b0 を読む（ルートシグネチャは b0 を全ステージ可視にしている）。リム計算用のカメラ位置が入っている。
cbuffer VSPerDraw : register(b0)
{
    float4x4 g_world;
    float4x4 g_viewProj;
    float4x4 g_oldWorldViewProj;
    float4 g_cameraPos; // xyz=カメラ世界座標, w=リム強度
};

cbuffer PSMaterial : register(b1)
{
    float4 g_diffuse;
    float4 g_filterParams; // x:明るさ, y:ボケ具合
};

// リムライトの調整値
static const float3 RIM_COLOR = float3(1.0, 1.0, 1.0); // リムの色（白）
static const float  RIM_POWER = 5.0;                   // 縁の鋭さ（大きいほど細い）
static const float  RIM_INTENSITY = 0.45;              // リムの強さ

// 頂点シェーダーの出力と合わせる
struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 worldNormal : TEXCOORD2; // ★追加: リムライト用のワールド法線

    // ★追加: ベロシティ用座標
    float4 curPos : TEXCOORD3;
    float4 prevPos : TEXCOORD4;
};

// ★追加: MRT用出力構造体
struct PSOutput
{
    float4 color : SV_TARGET0; // 今までの色 (RenderTargets[0])
    float4 velocity : SV_TARGET1; // 速度マップ (RenderTargets[1])
};

// 戻り値を float4 から PSOutput に変更
PSOutput PSMain(PSInput pin)
{
    PSOutput output;

    // --- 既存の色計算処理 ---
    float4 tex = g_albedo.SampleBias(g_samp, pin.uv, g_filterParams.y);
    float4 finalColor = (tex * g_diffuse) * g_filterParams.x;

  // ==========================================
    // ★★★ ここを追加：自己発光（エミッション）の限界突破 ★★★
    // ==========================================
    float emission = g_filterParams.w;
    if (emission > 0.0f)
    {
        // ★修正: テクスチャの色(finalColor)が黒くても、
        // マテリアル自体の色(g_diffuse)を足して無理やり光を引っ張り出す！
        finalColor.rgb += (finalColor.rgb + g_diffuse.rgb) * emission;
    }

    // 全体の明るさフィルターを適用
    finalColor *= g_filterParams.x;

    // ==========================================
    // ★リムライト：視線と法線が垂直なほど縁を光らせる（フレネル風）
    //   g_cameraPos.w がリム強度（キャラ=1.0、ステージ/地面=0）
    // ==========================================
    if (g_cameraPos.w > 0.0f)
    {
        float3 N = normalize(pin.worldNormal);
        float3 V = normalize(g_cameraPos.xyz - pin.worldPos); // 表面→カメラ方向
        float rim = 1.0f - saturate(dot(N, V));               // 縁ほど1に近づく
        rim = pow(rim, RIM_POWER);
        finalColor.rgb += RIM_COLOR * rim * RIM_INTENSITY * g_cameraPos.w;
    }

    // --- フォグ計算 (変更なし) ---
    //float3 eyePos = float3(0.0f, 20.0f, -100.0f);
    //float dist = distance(pin.worldPos, eyePos);
    //float fogStart = 100.0f;
    //float fogEnd = 250.0f;
    //float3 fogColor = float3(1.0f, 1.0f, 1.0f);
    //float fogFactor = saturate((dist - fogStart) / (fogEnd - fogStart));
    //finalColor.rgb = lerp(finalColor.rgb, fogColor, fogFactor);

    // ★色を構造体にセット
    output.color = finalColor;


    // ==========================================
    // ★追加: ベロシティ(速度)の計算 (変更なし)
    // ==========================================
    
    // 1. 透視除算を行い、NDC座標 (-1.0 ~ 1.0) に変換
    float2 curNDC = pin.curPos.xy / pin.curPos.w;
    float2 prevNDC = pin.prevPos.xy / pin.prevPos.w;

    // 2. 差分を取る (これが移動量)
    float2 velocity = (curNDC - prevNDC);

    // 3. UV空間 (0.0 ~ 1.0) 用に補正
    // X軸は半分に、Y軸はDirectXの座標系(上がマイナス)に合わせて反転して半分に
    velocity.x *= 0.5f;
    velocity.y *= -0.5f;

    // 4. 速度バッファに出力 (RとG成分を使用)
    // エフェクト等でマスクしたい場合は zw 成分などをフラグに使うこともあります
    // ★変更: velocityのw成分(Alpha)にフラグを出力
    // 1.0なら対象、0.0なら除外
    output.velocity = float4(velocity, 0.0f, g_filterParams.z);
    
    return output;
}