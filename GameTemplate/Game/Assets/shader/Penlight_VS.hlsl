// ペンライト（観客）用 頂点シェーダー：GPUインスタンシングで多数の発光ビルボードを描く。
// クアッド1枚(4頂点)を共有し、SV_InstanceID でインスタンスバッファから位置・色・位相を取得する。

cbuffer PenlightGlobal : register(b0)
{
    float4x4 g_viewProj;    // ビュー射影
    float4   g_camRight;    // カメラの右ベクトル(xyz)。ビルボードの横方向に使う
    float4   g_timeParams;  // x:時間（揺れアニメ用）
};

// 1本分のインスタンスデータ
struct PenlightInstance
{
    float3 position; // 根元のワールド座標
    float  phase;    // 揺れの位相オフセット（本ごとにバラす）
    float4 color;    // 色(rgb) + 強さ(a)
};
StructuredBuffer<PenlightInstance> g_instances : register(t0);

struct VSInput
{
    float2 localPos : POSITION; // クアッドのローカル座標 x:-0.5..0.5, y:0..1（0=根元,1=先端）
    float2 uv       : TEXCOORD0;
    uint   instId   : SV_InstanceID;
};

struct VSOutput
{
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 color : TEXCOORD1;
};

VSOutput VSMain(VSInput vin)
{
    VSOutput vout;

    PenlightInstance inst = g_instances[vin.instId];

    // 揺れ：先端ほど大きく左右に揺れる（根元は固定）。本ごとに位相と速度を少し変える。
    float t = g_timeParams.x;
    float sway = sin(t * 4.0f + inst.phase) * 0.35f * vin.localPos.y;

    const float WIDTH  = 1.8f; // ペンライトの太さ
    const float HEIGHT = 7.0f;  // ペンライトの高さ

    float3 right = normalize(g_camRight.xyz);   // 横はカメラ正対
    float3 up    = float3(0.0f, 1.0f, 0.0f);    // 縦はワールド上方向（直立させる）

    // 根元(position)から、右方向に太さ＋揺れ、上方向に高さ分を足してクアッドを立てる
    float3 worldPos = inst.position
        + right * (vin.localPos.x * WIDTH + sway)
        + up    * (vin.localPos.y * HEIGHT);

    vout.pos = mul(float4(worldPos, 1.0f), g_viewProj);
    vout.uv = vin.uv;
    vout.color = inst.color;
    return vout;
}
