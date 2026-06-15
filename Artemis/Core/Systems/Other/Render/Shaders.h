#pragma once

namespace Shaders
{
    inline constexpr const char* kMapSource = R"(
cbuffer Camera : register(b0)
{
    row_major float4x4 ViewProj;
};

cbuffer Tint : register(b1)
{
    float Alpha;
    float3 _pad;
};

struct VSIn  { float3 pos : POSITION; float3 color : COLOR; };
struct VSOut { float4 pos : SV_POSITION; float3 color : COLOR; };

VSOut VSMain(VSIn input)
{
    VSOut o;
    o.pos = mul(float4(input.pos, 1.0f), ViewProj);
    o.color = input.color;
    return o;
}

float4 PSMain(VSOut input) : SV_TARGET
{
    return float4(input.color, Alpha);
}
)";
}