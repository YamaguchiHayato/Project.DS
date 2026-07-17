/*!
 * @brief	・ｽX・ｽv・ｽ・ｽ・ｽC・ｽg・ｽp・ｽﾌシ・ｽF・ｽ[・ｽ_・ｽ[・ｽB
 */

cbuffer cb : register(b0){
	float4x4 mvp;		//・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽh・ｽr・ｽ・ｽ・ｽ[・ｽv・ｽ・ｽ・ｽW・ｽF・ｽN・ｽV・ｽ・ｽ・ｽ・ｽ・ｽs・ｽ・ｽB
	float4 mulColor;	//・ｽ・ｽZ・ｽJ・ｽ・ｽ・ｽ[・ｽB
};
struct VSInput{
	float4 pos : POSITION;
	float2 uv  : TEXCOORD0;
};

struct PSInput{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

Texture2D<float4> colorTexture : register(t0);	//・ｽJ・ｽ・ｽ・ｽ[・ｽe・ｽN・ｽX・ｽ`・ｽ・ｽ・ｽB
sampler Sampler : register(s0);

PSInput VSMain(VSInput In) 
{
	PSInput psIn;
	psIn.pos = mul( mvp, In.pos );
	psIn.uv = In.uv;
	return psIn;
}
float4 PSMain( PSInput In ) : SV_Target0
{
	return colorTexture.Sample(Sampler, In.uv) * mulColor;
}