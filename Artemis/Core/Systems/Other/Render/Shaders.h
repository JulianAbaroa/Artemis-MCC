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
    float _pad0;
    float2 _pad1;
};

struct VSIn  { float3 pos : POSITION; float3 color : COLOR; };
struct VSOut
{
    float4 pos      : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 color    : COLOR;
};

VSOut VSMain(VSIn input)
{
    VSOut o;
    o.pos = mul(float4(input.pos, 1.0f), ViewProj);
    o.worldPos = input.pos;
    o.color = input.color;
    return o;
}

float4 PSMain(VSOut input) : SV_TARGET
{
    float3 dx = ddx(input.worldPos);
    float3 dy = ddy(input.worldPos);
    float3 n = normalize(cross(dx, dy));

    float3 lightDir1 = normalize(float3(0.4f, 0.6f, 0.7f));
    float3 lightDir2 = normalize(float3(-0.5f, -0.3f, 0.2f));

    float diff1 = saturate(dot(n, lightDir1) * 0.5f + 0.5f);
    float diff2 = saturate(dot(n, lightDir2) * 0.5f + 0.5f) * 0.3f;

    float ambient = 0.5f;

    float lighting = ambient + diff1 * 0.5f + diff2;
    lighting = saturate(lighting);

    float3 c = input.color * lighting;
    return float4(c, Alpha);
}
)";
}