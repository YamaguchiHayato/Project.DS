cbuffer HPGaugeCB : register(b1)
{
    float hpRate;
    float startAngle;
    float maxAngle;
    float innerRadius; // ここをわっかの開始地点にする
    float auraRange; // 広がる幅 (Updateの第6引数)
    float time; // damageFlashの変数に time を渡す
    float2 padding; // 16バイト境界合わせ
};

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

// 定数バッファ b0 (エンジン標準) を追加
cbuffer SpriteCB : register(b0)
{
    float4x4 matWorld;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    // ★行列を掛けて、SetPositionで設定した位置に移動させる
    output.pos = mul(matWorld, input.pos);
    output.uv = input.uv;
    return output;
}



// --- ピクセルシェーダー ---
float4 PSMain(PSInput input) : SV_Target
{
    // 1. 中心(0.5, 0.5)を基準にした距離と角度
    float2 d = input.uv - 0.5;
    float dist = length(d);
    float angle = atan2(-d.y, d.x);

    // 2. 描画制限
    // HPが100%未満、または下半分の場合は何も描画しない
    if (angle < 0.0f || angle > 3.14159f)
    {
        discard;
    }
    
    // 2. ★追加：黒枠より内側をカット
    // innerRadius（わっかの開始地点）より内側には何も描画しない
    // もし切り抜きすぎ・足りない場合は、この数値を innerRadius - 0.05f などで微調整してください
    if (dist < innerRadius+0.13f)
        discard;

    // 3. 波紋（広がっていくわっか）の計算
    // speed: 広がる速さ / pulse: 0.0〜1.0 のループ
    float speed = 1.3f;
    float pulse1 = frac(time * speed); // 1つ目の波
    float pulse2 = frac(time * speed + 0.5f); // 2つ目の波（0.5周期ずらす）

    // 輪の現在の半径
    float radius1 = innerRadius + (pulse1 * auraRange);
    float radius2 = innerRadius + (pulse2 * auraRange);

    // 4. 光の強度の計算 (exp関数で鋭い光の輪を作る)
    // 60.0f の値を大きくすると線が細く、小さくすると太くなります(今8)
    float glow1 = exp(-abs(dist - radius1) * 8.0f);
    float glow2 = exp(-abs(dist - radius2) * 8.0f);

    // 外側に広がるほど、また時間の経過(pulse)に伴ってフェードアウトさせる
    glow1 *= (1.0f - pulse1);
    glow2 *= (1.0f - pulse2);

    // 5. 色の合成
    // 金色〜オレンジ系の光 (1.0, 0.8, 0.3)
    float3 finalRGB = float3(0.4, 1.2, 0.1) * (glow1 + glow2) * 1.5f;
    float finalAlpha = (glow1 + glow2);
    
    // ★追加：スプライトの四角い縁を消す処理 (円形フェード)
    // 中心からの距離が 0.45 を超えたら徐々に消し、0.5（板の端）で完全に透明にする
    float edgeFade = saturate((0.5f - dist) * 10.0f);
    finalAlpha *= edgeFade;
    
    // 最終出力
    return float4(finalRGB * edgeFade, finalAlpha);
}