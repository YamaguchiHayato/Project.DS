cbuffer VSPerDraw : register(b0)
{
    float4x4 g_world;
    float4x4 g_viewProj;
    // ★追加: 1フレーム前の World * View * Proj 行列
    float4x4 g_oldWorldViewProj;
    // ★追加: xyz=カメラ世界座標(リム計算用), w=リム強度
    float4 g_cameraPos;
};

StructuredBuffer<float4x4> g_boneMatrices : register(t1);

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    uint4 boneIdx : BLENDINDICES;
    float4 boneW : BLENDWEIGHT;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 worldNormal : TEXCOORD2; // ★追加: リムライト用のワールド法線

    // ★追加: ベロシティ計算用の座標
    float4 curPos : TEXCOORD3; // 現在のクリップ空間座標
    float4 prevPos : TEXCOORD4; // 1フレ前のクリップ空間座標
};

VSOutput VSMain(VSInput vin)
{
    VSOutput vout;

    float4 skinned = float4(0, 0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0); // ★追加: スキニング後の法線

    // スキニング計算
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint idx = vin.boneIdx[i];
        float w = vin.boneW[i];
        if (w <= 0)
            continue;

        float4x4 m = g_boneMatrices[idx];
        skinned += mul(float4(vin.pos, 1.0f), m) * w;
        // 法線は平行移動を無視（w=0）して回転だけ適用
        skinnedNormal += mul(float4(vin.normal, 0.0f), m).xyz * w;
    }

    // --- 現在の座標計算 ---
    float4 worldPos = mul(skinned, g_world); // ワールド座標
    vout.pos = mul(worldPos, g_viewProj); // クリップ座標 (SV_POSITION)

    vout.worldPos = worldPos.xyz;
    // ★法線もワールドへ（平行移動無視）。正規化はPSで行う
    vout.worldNormal = mul(float4(skinnedNormal, 0.0f), g_world).xyz;
    vout.uv = vin.uv;

    // --- ★追加: ベロシティ用の座標保存 ---
    // 1. 現在のクリップ座標を保存 (vout.posと同じもの)
    vout.curPos = vout.pos;

    // 2. 過去のクリップ座標を計算
    // スキニング後のローカル座標(skinned)に対して、過去の行列(World*View*Proj)を掛ける
    vout.prevPos = mul(skinned, g_oldWorldViewProj);

    return vout;
}