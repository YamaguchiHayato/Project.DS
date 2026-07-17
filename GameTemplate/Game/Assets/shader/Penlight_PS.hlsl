// ペンライト用 ピクセルシェーダー：テクスチャ無しで手続き的に発光させる。
// 中央が明るく、横・先端に向かってフェードする「光る棒」。加算合成＆ブルームで光って見える。
// メインRTはMRT(色＋速度)なので2ターゲット出力する。

struct PSInput
{
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;   // x:0..1(横), y:0..1(縦, 0=根元,1=先端)
    float4 color : TEXCOORD1;   // rgb=色, a=強さ
};

struct PSOutput
{
    float4 color    : SV_TARGET0; // 色
    float4 velocity : SV_TARGET1; // 速度マップ（ペンライトはモーションブラー対象外）
};

PSOutput PSMain(PSInput pin)
{
    PSOutput o;

    // 横方向：中央(0.5)で1.0、端で0へ。コア＋ふんわりグロー。
    float h = 1.0f - saturate(abs(pin.uv.x - 0.5f) * 2.0f);
    float glow = pow(h, 2.0f);

    // 縦方向：根元と先端を少しだけフェードして棒の端を柔らかく
    float v = smoothstep(0.0f, 0.08f, pin.uv.y) * smoothstep(1.0f, 0.85f, pin.uv.y);

    float intensity = glow * v * pin.color.a;

    o.color = float4(pin.color.rgb * intensity, intensity);

    // 速度は出さない（w=0でモーションブラー除外）
    o.velocity = float4(0.0f, 0.0f, 0.0f, 0.0f);
    return o;
}
