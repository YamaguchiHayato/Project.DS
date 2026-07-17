// --- Spriteクラスが自動で送ってくる基本データ (b0) ---
cbuffer cbSprite : register(b0)
{
    matrix g_mvp; // MVP行列 (World * View * Proj)
    float4 g_mulColor; // 乗算カラー
    float4 g_screen; // スクリーンパラメータ
};

// --- 今回追加するユーザー拡張データ (b1) ---
// DiamondWipeクラスから送るデータ
cbuffer cbDiamond : register(b1)
{
    float g_progress; // 0.0(開始) -> 1.0(終了)
    float g_tiling; // ひし形の数 (例: 16.0)
    float2 g_padding; // アライメント用
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

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

// 頂点シェーダー (Sprite標準のものとほぼ同じ)
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.pos = mul(input.pos, g_mvp);
    output.uv = input.uv;
    return output;
}

// ピクセルシェーダー (ひし形ロジック)
float4 PSMain(PSInput input) : SV_TARGET
{
    // 画像の色を取得
    float4 color = g_texture.Sample(g_sampler, input.uv);
    color *= g_mulColor; // 色成分を乗算

    // --- ひし形ワイプ計算 ---
    
    // 1. グリッド分割
    float2 tileUV = input.uv * g_tiling;
    // アスペクト比補正 (画面が横長なのでXを補正したい場合)
    // float aspect = g_screen.z / g_screen.w;
    // tileUV.x *= aspect; 

    float2 subUV = frac(tileUV) - 0.5; // -0.5 ～ 0.5
    
    // 2. マンハッタン距離 (ひし形)
    float dist = abs(subUV.x) + abs(subUV.y);
    
    // 3. 遅延計算 (左下 -> 右上)
    // UVは左上が(0,0)なので、左下は(0,1)。計算しやすいように調整
    // 左下(0,1)付近からスタートさせたい場合:
    float delay = input.uv.x + (1.0 - input.uv.y);
    
    // 4. しきい値の移動
    // progressが動くと、thresholdが大きくなり画面を覆っていく
    float threshold = (g_progress * 3.5) - delay;
    
    // 5. マスク生成
    // dist(中心からの距離) が threshold より小さい部分を表示
    // smoothstepで境界を滑らかに
    float mask = 1.0 - smoothstep(threshold - 0.05, threshold + 0.05, dist);
    
    // アルファ値にマスクを適用
    color.a *= mask;
    
    return color;
}