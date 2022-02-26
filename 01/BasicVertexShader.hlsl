/*-------------------------------------------------

    [BasicVertexShader.hlsl]
    Author : èoçá„ƒëæ

--------------------------------------------------*/
struct VSInput
{
    float4 Position : POSITION;
    float4 Normal : NORMAL;
    uint IntstanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

struct InstanceData
{
    float4x4 World;
    float4 Color;
};

cbuffer ShaderParamter : register(b0)
{
    float4x4 View;
    float4x4 Projection;
}

cbuffer InstanceParameter : register(b1)
{
    InstanceData data[500];
}

VSOutput main(VSInput In)
{
    VSOutput output;
    uint index = In.IntstanceID;
    float4x4 world = data[index].World;
    float4x4 matrixWVP = mul(world, mul(View, Projection));
    output.Position = mul(In.Position, matrixWVP);
    output.Color = saturate(dot(In.Normal, float3(0, 1, 0))) * 0.5 + 0.5;
    output.Color.a = 1;
    output.Color *= data[index].Color;
    return output;
}