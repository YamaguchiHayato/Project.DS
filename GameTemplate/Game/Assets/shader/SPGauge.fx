// --- 定数バッファ (b0: エンジン標準) ---
cbuffer SpriteCB : register(b0)
{
    float4x4 matWorld;
};

// --- 定数バッファ (b1: ユーザー拡張) ---
cbuffer HPGaugeCB : register(b1)
{
    float hpRate; // SPの割合 (0.0~1.0)
    float startAngle; // C++から渡される開始角 (3.14159)
    float maxAngle; // C++から渡される終了角 (0.0)
    float innerRadius; // 内径
    float outerRadius; // 外径
    float damageFlash; // 光らせる演出用
    float2 centerPos; // パディング込み
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

// 頂点シェーダー
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.pos = mul(matWorld, input.pos);
    output.uv = input.uv;
    return output;
}

// 定数バッファ等の定義は省略（既存のものを使用）

float4 PSMain(PSInput input) : SV_Target
{
    // 1. テクスチャ取得
    float4 texColor = g_texture.Sample(g_sampler, input.uv);

    // 2. 角度計算 (UV 0.5基準)
    float2 d = input.uv - 0.5; // 中心からのベクトル
    float dist = length(d);
    float angle = atan2(-d.y, d.x);

    // 3. 境界計算 (左:3.14 から 右:0.0)
    float currentLimitAngle = 3.14159f * (1.0f - hpRate);

    // ★境界線（直線）の方向計算★
    float2 lineDir = float2(cos(currentLimitAngle), -sin(currentLimitAngle));
    float2 normalDir = float2(lineDir.y, -lineDir.x); // 垂直ベクトル

    // 中心からの距離dと垂直ベクトルの内積で、境界線からの最短距離が出る
    float distanceFromLine = dot(d, normalDir);
    // ★重要：中心から見て境界線の「向き」と同じ側にいるか判定（反対側の線を消す）
    float sideCheck = dot(d, lineDir);

    // 境界線の太さ（一定）
    float lineWidth = 0.009;
    float hubRadius = 0.04; // ★中心の小さい半円の半径（好みに合わせて調整）★
    
    // 4. ★膜の制御ロジック★
    if (angle > currentLimitAngle)
    {
        // 【SPが溜まった場所：左側】 完全に透明にしてキャラを露出
        texColor.a = 0.0f;
    }
    else
    {
        // 【SPがまだ溜まっていない場所：右側】 キャラを隠す膜
        texColor.rgb = float3(0.2, 0.2, 0.2); // 暗いグレー
        texColor.a = 0.7f;
    }
    
    
    // 5. 境界線の描画
    // 条件1: 線からの距離が近い
    // 条件2: hpRateが0より大きく、1.0より小さい
    // 条件3: sideCheck > 0 (中心から境界線方向のみ描画し、逆方向をカット)
    if (abs(distanceFromLine) < lineWidth && hpRate > 0.001f && hpRate < 0.999f && sideCheck > 0.0f)
    {
        // お皿の形状（ドーナツ幅と角度）の中だけ上書き
        if (dist >= innerRadius && dist <= outerRadius && angle >= 0.0f && angle <= 3.14159f)
        {
            texColor.rgb = float3(1.0, 1.0, 1.0); // 白い線
            texColor.a = 1.0f;
        }
    }
    
    // 6. ★中心の小さな半円（ハブ）を上書き描画★
    // 角度が 0~180度の範囲内で、かつ半径が hubRadius 以内なら白く塗る
    if (dist < hubRadius && angle >= 0.0f && angle <= 3.14159f)
    {
        if (hpRate > 0.001f)
        {
            // 1%以上なら「点灯」して白くなる
            texColor.rgb = float3(1.0, 1.0, 1.0);
            texColor.a = 1.0f;
        }
        else
        {
            // 0%なら「消灯」状態（膜と同じグレー）
            texColor.rgb = float3(0.2, 0.2, 0.2);
            texColor.a = 0.7f;
        }
    }
    
    // 7. 最終的な形状切り抜き
    if (angle < 0.0f || angle > 3.14159f)
        discard;

    // ★修正ポイント：ハブがある場所(dist < hubRadius)は discard しないようにする★
    if (dist > outerRadius || (dist < innerRadius && dist >= hubRadius))
    {
        discard;
    }

    return texColor;
}