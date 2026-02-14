struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
};

cbuffer MaterialConstants : register(b0)
{
    float4 gAlbedo;
};

Texture2D gBaseColorTex : register(t0);
SamplerState gLinearSampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = gBaseColorTex.Sample(gLinearSampler, input.uv);
    return float4(input.color, 1.0f) * texColor * gAlbedo;
}
