export module Platform.Render.System:Shaders;

export namespace Platform::Render::System::Shaders
{
    // HLSL source of the map pipeline, shared by every pass.
    // Vertex shaders: VSMain (Vertex), VSInstanced (SphereInstance) and VSInstancedMesh (MeshInstance).
    // Pixel shader: PSMain, flat shading from the screen-space derivatives and two fixed lights.
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

struct VSInstIn
{
    float3 pos    : POSITION;
    float4 center : INST_CENTER;
    float3 color  : INST_COLOR;
};

VSOut VSInstanced(VSInstIn input)
{
    VSOut o;
    float3 world = input.center.xyz + input.pos * input.center.w;
    o.pos = mul(float4(world, 1.0f), ViewProj);
    o.worldPos = world;
    o.color = input.color;
    return o;
}

struct VSMeshIn
{
    float3 pos   : POSITION;
    float4 row0  : INST_ROW0;
    float4 row1  : INST_ROW1;
    float4 row2  : INST_ROW2;
    float3 color : INST_COLOR;
};

VSOut VSInstancedMesh(VSMeshIn input)
{
    float4 p = float4(input.pos, 1.0f);
    float3 world = float3(dot(input.row0, p), dot(input.row1, p), dot(input.row2, p));

    VSOut o;
    o.pos = mul(float4(world, 1.0f), ViewProj);
    o.worldPos = world;
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