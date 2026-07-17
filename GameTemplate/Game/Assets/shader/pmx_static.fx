cbuffer Transform : register(b0)
{
    float4x4 gWorld;
    float4x4 gView;
    float4x4 gProj;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(VSInput i)
{
    VSOutput o;
    float4 wpos = mul(float4(i.pos, 1.0f), gWorld);
    float4 vpos = mul(wpos, gView);
    o.svpos = mul(vpos, gProj);
    o.uv = i.uv;
    return o;
}


Texture2D gAlbedo : register(t0);
SamplerState gSmp : register(s0);

float4 PSMain(float2 uv : TEXCOORD0) : SV_Target
{
    float4 c = gAlbedo.Sample(gSmp, uv);
    return c;
}
