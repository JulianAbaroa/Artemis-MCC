export module Platform.Render.System:Shaders;

export namespace Platform::Render::System::Shaders
{
    inline constexpr const char* k_MapSource = R"(
cbuffer Camera : register(b0)
{
    row_major float4x4 ViewProj;
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

    return float4(input.color * lighting, 1.0f);
}
)";
}