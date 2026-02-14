struct PSInput
{
    float4 position : SV_POSITION;
    float3 normalPacked : COLOR;
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
    float3 n = normalize(input.normalPacked * 2.0f - 1.0f);
    float3 l = normalize(float3(0.35f, 0.75f, -0.55f));
    float ndl = saturate(dot(n, l));
    float lighting = 0.18f + ndl * 0.82f;

    float4 texColor = gBaseColorTex.Sample(gLinearSampler, input.uv);
    return texColor * gAlbedo * float4(lighting, lighting, lighting, 1.0f);
}

