struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;     // Packed normal from CPU path (0..1)
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normalPacked : COLOR;
    float2 uv : TEXCOORD0;
};

cbuffer FrameConstants : register(b0)
{
    row_major float4x4 gMVP;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), gMVP);
    output.normalPacked = input.color;
    output.uv = input.uv;
    return output;
}

