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

// --- ピクセルシェーダー ---
// --- ピクセルシェーダー ---
float4 PSMain(PSInput input) : SV_Target
{
    float2 center = float2(0.5, 0.5);
    float2 pos = input.uv - center;
    float dist = length(pos);
    
    // 1. 角度の基本範囲（上 1.57 ～ 左 3.14）
    float angle = atan2(-pos.y, pos.x);

    // 範囲外をカット
    if (angle < startAngle || angle > maxAngle)
        discard;

    // 2. HPによる切り抜き
    // 真上(1.57)から開始し、HPが減るほど境界が真左(3.14)へ移動
    float currentLimitAngle = lerp(maxAngle, startAngle, hpRate);

    // 境界より「上（数値が小さい）」の部分を捨てることで、
    // 上から左へ削れていく動きにする
    if (angle < currentLimitAngle)
    {
        discard;
    }

    // ★★★ ここが消えていたので復活！ ★★★
    // 半径による切り抜き（これでドーナツ型になる）
    if (dist < innerRadius || dist > outerRadius)
    {
        discard;
    }
    
 // --- 3. カラー出力部分の修正 ---
    float4 texColor = g_texture.Sample(g_sampler, input.uv);
    
    float3 healthColor;
    if (innerRadius == 0.0f && outerRadius == 0.39)
    {
        healthColor = float3(1.0, 1.0, 1.0); //SPゲージの円形表示の背景は白色
    }
    // 内径が0の場合（円形表示）は一番後ろの半円のため特別処理。
    else if (innerRadius == 0.0f)
    {
        healthColor = float3(0.0, 0.0, 0.0); //円形表示の背景は黒色
        texColor.a = 0.5; //背景用は透明度50%
    }
    // ==========================================
    // ★追加：シールド用フラグ（-2.0f以下なら水色にする）
    // ==========================================
    else if (damageFlash <= -2.0f)
    {
        healthColor = float3(0.0, 0.8, 1.0); // シールドは水色
    }
    //HPゲージの背景用かどうかの判定
    else if (damageFlash < 0.0f)
    {
        healthColor = float3(0.8, 0.8, 0.8); //背景用は灰色
    }
    // HP本体ならHP量に応じて色を決定
    else if (hpRate < 0.3f)
    {
        healthColor = float3(1.0, 0.0, 0.0); // 30%未満：赤色
    }
    else if (hpRate < 0.7f)
    {
        healthColor = float3(1.0, 1.0, 0.0); // 80%未満：黄色
    }
    else
    {
        healthColor = float3(0.0, 1.0, 0.0); // それ以上：緑色
    }
    
    // ==========================================
    // ★修正：マイナスの値で色がおかしくならないよう、
    // ダメージフラッシュ値を 0.0 ～ 1.0 に制限(saturate)する
    // ==========================================
    float flashAmount = saturate(damageFlash);
    float3 finalColor = lerp(healthColor * texColor.rgb, float3(1.0, 1.0, 1.0), flashAmount);

    return float4(finalColor, texColor.a);
}