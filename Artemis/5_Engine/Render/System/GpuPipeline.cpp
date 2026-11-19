module;

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#pragma comment(lib, "d3dcompiler.lib")

module Engine.Render.System;
import :GpuPipeline;
import std;

namespace
{
    using Microsoft::WRL::ComPtr;

    using Common::Log::System::Log;

    using Engine::Render::Type::MaskConstants;
    using Engine::Render::Type::MakeMaskConstants;
    using Engine::Render::Type::MaskPairKind;

    using Engine::Render::Type::k_MaskBufferSlot;
    using Engine::Render::Type::k_MaskSlotCount;
    using Engine::Render::Type::MaskPostSlot;
    using Engine::Render::Type::MaskPreSlot;

    // The copies of the pairs are bound together from slot 0, before and after of each pair in the order of MaskPairKind.
    static_assert(MaskPreSlot(MaskPairKind::DepthB) == 0 && MaskPostSlot(MaskPairKind::DepthB) == 1 &&
        MaskPostSlot(MaskPairKind::DepthZero) == k_MaskSlotCount - 1);

    // Depth bias of the DepthOnly surface. The depth is reversed (near is 1, far is 0), so negative terms push the surface away from the camera.
    // The wireframe of the same mesh then wins the depth test without being pulled through nearby walls. The bias is in units of the float
    // depth buffer (scaled by the exponent of the depth of the triangle, so it holds near and far), and the slope term covers grazing angles.
    constexpr int k_DepthOnlyDepthBias{ 128 };
    constexpr float k_DepthOnlySlopeBias{ 2.0f };

    // HLSL source of the map pipeline, shared by every pass.
    // Vertex shaders: VSMain (Vertex), VSInstanced (SphereInstance) and VSInstancedMesh (MeshInstance).
    // Pixel shader: PSMain, flat shading from the screen-space derivatives and two fixed lights. It first skips the pixels of the
    // first person arms of the game (IsFirstPersonPixel): the texels whose depth differs between the two copies of any pair taken
    // around the passes that draw them. The copies are t0 to t3 (depth 0xB, depth 0) and the sizes and flags b1, bound by UpdateMask.
    constexpr const char* k_MapSource = R"(
cbuffer Camera : register(b0)
{
    row_major float4x4 ViewProj;
};

cbuffer Mask : register(b1)
{
    uint2 DepthBSize;
    uint2 DepthZeroSize;
    uint2 TargetSize;
    uint  ValidFlags;
    float ColorScale;
};

Texture2D<float>  DepthBPre    : register(t0);
Texture2D<float>  DepthBPost   : register(t1);
Texture2D<float>  DepthZeroPre : register(t2);
Texture2D<float>  DepthZeroPost: register(t3);

// The texel of a copy of the given size that covers the pixel, as ScaleToMask does.
int3 MaskTexel(uint2 pixel, uint2 size)
{
    return int3(min(pixel * size / TargetSize, size - 1), 0);
}

// Whether the pixel is one the game draws its first person arms on: the depth changed in the pass 0xB or in the pass 0. The flags say which
// pairs were copied in the frame.
bool IsFirstPersonPixel(float4 screenPos)
{
    if (ValidFlags == 0) return false;

    uint2 pixel = uint2(screenPos.xy);

    if ((ValidFlags & 1) != 0)
    {
        int3 location = MaskTexel(pixel, DepthBSize);
        if (DepthBPre.Load(location) != DepthBPost.Load(location)) return true;
    }

    if ((ValidFlags & 2) != 0)
    {
        int3 location = MaskTexel(pixel, DepthZeroSize);
        if (DepthZeroPre.Load(location) != DepthZeroPost.Load(location)) return true;
    }

    return false;
}

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
    // The derivatives come first, so the early return below is not between a gradient and the rest of the shader.
    float3 dx = ddx(input.worldPos);
    float3 dy = ddy(input.worldPos);

    if (IsFirstPersonPixel(input.pos)) discard;

    float3 n = normalize(cross(dx, dy));

    float3 lightDir1 = normalize(float3(0.4f, 0.6f, 0.7f));
    float3 lightDir2 = normalize(float3(-0.5f, -0.3f, 0.2f));

    float diff1 = saturate(dot(n, lightDir1) * 0.5f + 0.5f);
    float diff2 = saturate(dot(n, lightDir2) * 0.5f + 0.5f) * 0.3f;

    float ambient = 0.5f;

    float lighting = ambient + diff1 * 0.5f + diff2;
    lighting = saturate(lighting);

    return float4(input.color * lighting * ColorScale, 1.0f);
}
)";

    // Compiles an entry point of the map shader source.
    // return: False if it fails. The compiler errors are logged.
    auto CompileShader(const char* entry, const char* target,
        Log& logs, ComPtr<ID3DBlob>& blob) -> bool
    {
        UINT flags = 0;
#if defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        const char* source = k_MapSource;

        ComPtr<ID3DBlob> errors{};
        const HRESULT result = D3DCompile(source, std::strlen(source),
            nullptr, nullptr, nullptr, entry, target, flags, 0,
            blob.GetAddressOf(), errors.GetAddressOf());

        if (FAILED(result))
        {
            std::string text = errors ?
                std::string(static_cast<const char*>(errors->GetBufferPointer())) :
                std::string("unknown error");

            logs.Message("[GpuPipeline] ERROR: {} shader failed to compile: {}", entry, text);
            return false;
        }

        return true;
    }
}

namespace Engine::Render::System
{
    auto GpuPipeline::Initialize(ID3D11Device* device) -> bool
    {
        if (m_IsReady) return true;

        if (!device)
        {
            m_Log.Message("[GpuPipeline] ERROR: No device.");
            return false;
        }

        if (!this->CompileShaders(device) || !this->CreateBuffers(device) ||
            !this->CreateStates(device))
        {
            this->Release();
            return false;
        }

        m_IsReady = true;
        m_Log.Message("[GpuPipeline] INFO: Ready.");
        return true;
    }

    auto GpuPipeline::CompileShaders(ID3D11Device* device) -> bool
    {
        ComPtr<ID3DBlob> vertexBlob{};
        ComPtr<ID3DBlob> pixelBlob{};
        ComPtr<ID3DBlob> instancedBlob{};
        ComPtr<ID3DBlob> meshBlob{};

        if (!CompileShader("VSInstanced", "vs_5_0", m_Log, instancedBlob)) return false;
        if (!CompileShader("VSInstancedMesh", "vs_5_0", m_Log, meshBlob)) return false;
        if (!CompileShader("VSMain", "vs_5_0", m_Log, vertexBlob)) return false;
        if (!CompileShader("PSMain", "ps_5_0", m_Log, pixelBlob)) return false;

        if (FAILED(device->CreateVertexShader(vertexBlob->GetBufferPointer(),
            vertexBlob->GetBufferSize(), nullptr, m_VertexShader.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: CreateVertexShader failed.");
            return false;
        }

        if (FAILED(device->CreatePixelShader(pixelBlob->GetBufferPointer(),
            pixelBlob->GetBufferSize(), nullptr, m_PixelShader.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: CreatePixelShader failed.");
            return false;
        }

        if (FAILED(device->CreateVertexShader(instancedBlob->GetBufferPointer(),
            instancedBlob->GetBufferSize(), nullptr, m_InstancedVertexShader.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: CreateVertexShader (instanced) failed.");
            return false;
        }

        const D3D11_INPUT_ELEMENT_DESC instancedLayout[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "INST_CENTER", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INST_COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 16,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

        if (FAILED(device->CreateInputLayout(instancedLayout, 3,
            instancedBlob->GetBufferPointer(), instancedBlob->GetBufferSize(),
            m_InstancedInputLayout.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: CreateInputLayout (instanced) failed.");
            return false;
        }

        if (FAILED(device->CreateVertexShader(meshBlob->GetBufferPointer(),
            meshBlob->GetBufferSize(), nullptr, m_MeshVertexShader.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: CreateVertexShader (mesh) failed.");
            return false;
        }

        const D3D11_INPUT_ELEMENT_DESC meshLayout[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "INST_ROW", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INST_ROW", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INST_ROW", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INST_COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 48,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

        if (FAILED(device->CreateInputLayout(meshLayout, 5,
            meshBlob->GetBufferPointer(), meshBlob->GetBufferSize(),
            m_MeshInputLayout.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: CreateInputLayout (mesh) failed.");
            return false;
        }

        const D3D11_INPUT_ELEMENT_DESC layout[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        if (FAILED(device->CreateInputLayout(layout, 2,
            vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
            m_InputLayout.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: CreateInputLayout failed.");
            return false;
        }

        return true;
    }

    auto GpuPipeline::CreateBuffers(ID3D11Device* device) -> bool
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(Matrix);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(device->CreateBuffer(&desc, nullptr, m_CameraBuffer.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Camera constant buffer failed.");
            return false;
        }

        D3D11_BUFFER_DESC maskDesc{};
        maskDesc.ByteWidth = sizeof(MaskConstants);
        maskDesc.Usage = D3D11_USAGE_DYNAMIC;
        maskDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        maskDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(device->CreateBuffer(&maskDesc, nullptr, m_MaskBuffer.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Mask constant buffer failed.");
            return false;
        }

        return true;
    }

    auto GpuPipeline::CreateStates(ID3D11Device* device) -> bool
    {
        D3D11_RASTERIZER_DESC raster{};
        raster.FillMode = D3D11_FILL_SOLID;
        raster.CullMode = D3D11_CULL_NONE;
        raster.FrontCounterClockwise = FALSE;
        raster.DepthClipEnable = TRUE;

        if (FAILED(device->CreateRasterizerState(&raster, m_Rasterizer.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Rasterizer state failed.");
            return false;
        }

        D3D11_RASTERIZER_DESC depthOnly{ raster };
        depthOnly.DepthBias = -k_DepthOnlyDepthBias;
        depthOnly.SlopeScaledDepthBias = -k_DepthOnlySlopeBias;

        if (FAILED(device->CreateRasterizerState(&depthOnly, m_DepthOnlyRasterizer.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Depth only rasterizer state failed.");
            return false;
        }

        D3D11_RASTERIZER_DESC wireframe{ raster };
        wireframe.FillMode = D3D11_FILL_WIREFRAME;

        if (FAILED(device->CreateRasterizerState(&wireframe, m_WireframeRasterizer.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Wireframe rasterizer state failed.");
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC depth{};
        depth.DepthEnable = TRUE;
        depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        depth.StencilEnable = FALSE;

        if (FAILED(device->CreateDepthStencilState(&depth, m_DepthState.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Depth state failed.");
            return false;
        }

        D3D11_BLEND_DESC blend{};
        blend.RenderTarget[0].BlendEnable = FALSE;
        blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(device->CreateBlendState(&blend, m_BlendState.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Blend state failed.");
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC overlay{ depth };
        overlay.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        if (FAILED(device->CreateDepthStencilState(&overlay, m_OverlayDepthState.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Overlay depth state failed.");
            return false;
        }

        D3D11_BLEND_DESC translucent{};
        auto& target = translucent.RenderTarget[0];
        target.BlendEnable = TRUE;
        target.SrcBlend = D3D11_BLEND_BLEND_FACTOR;
        target.DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
        target.BlendOp = D3D11_BLEND_OP_ADD;
        target.SrcBlendAlpha = D3D11_BLEND_ONE;
        target.DestBlendAlpha = D3D11_BLEND_ZERO;
        target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(device->CreateBlendState(&translucent, m_TranslucentBlendState.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: Translucent blend state failed.");
            return false;
        }

        D3D11_BLEND_DESC noColor{};
        noColor.RenderTarget[0].BlendEnable = FALSE;
        noColor.RenderTarget[0].RenderTargetWriteMask = 0;

        if (FAILED(device->CreateBlendState(&noColor, m_NoColorBlendState.GetAddressOf())))
        {
            m_Log.Message("[GpuPipeline] ERROR: No color blend state failed.");
            return false;
        }

        return true;
    }

    auto GpuPipeline::UpdateCamera(ID3D11DeviceContext* context,
        const Matrix& viewProjection) -> void
    {
        if (!context || !m_CameraBuffer) return;

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context->Map(m_CameraBuffer.Get(), 0,
            D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, viewProjection.data(), sizeof(Matrix));
            context->Unmap(m_CameraBuffer.Get(), 0);
        }
    }

    auto GpuPipeline::UpdateMask(ID3D11DeviceContext* context, const MaskContext& mask, std::uint32_t targetWidth, std::uint32_t targetHeight,
        float colorScale) -> void
    {
        if (!context || !m_MaskBuffer) return;

        const MaskConstants constants{ MakeMaskConstants(mask, targetWidth, targetHeight, colorScale) };

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context->Map(m_MaskBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;

        std::memcpy(mapped.pData, &constants, sizeof(constants));
        context->Unmap(m_MaskBuffer.Get(), 0);

        ID3D11Buffer* buffer{ m_MaskBuffer.Get() };
        context->PSSetConstantBuffers(k_MaskBufferSlot, 1, &buffer);

        // A pair that is not used binds nothing, so the shader never reads a stale copy.
        ID3D11ShaderResourceView* views[k_MaskSlotCount]{};

        for (std::size_t i{ 0 }; i < mask.Pairs.size(); ++i)
        {
            if ((constants.ValidFlags & (1u << i)) == 0) continue;

            const MaskPairKind kind{ static_cast<MaskPairKind>(i) };
            views[MaskPreSlot(kind)] = mask.Pairs[i].Pre;
            views[MaskPostSlot(kind)] = mask.Pairs[i].Post;
        }

        context->PSSetShaderResources(0, k_MaskSlotCount, views);
    }

    auto GpuPipeline::ClearMask(ID3D11DeviceContext* context) -> void
    {
        if (!context || !m_MaskBuffer) return;

        ID3D11ShaderResourceView* views[k_MaskSlotCount]{};
        context->PSSetShaderResources(0, k_MaskSlotCount, views);
    }

    auto GpuPipeline::Bind(ID3D11DeviceContext* context, VertexLayout layout,
        SurfaceMode surface, float alpha) -> void
    {
        if (!context || !m_IsReady) return;

        const bool isTranslucent = surface == SurfaceMode::Translucent;
        const float blendFactor[4]{ alpha, alpha, alpha, alpha };

        ID3D11BlendState* blendState = m_BlendState.Get();
        if (isTranslucent) blendState = m_TranslucentBlendState.Get();
        else if (surface == SurfaceMode::DepthOnly) blendState = m_NoColorBlendState.Get();

        const bool writesDepth = surface == SurfaceMode::Opaque || surface == SurfaceMode::DepthOnly;

        context->OMSetBlendState(blendState, blendFactor, 0xffffffff);
        context->OMSetDepthStencilState(writesDepth ? m_DepthState.Get() : m_OverlayDepthState.Get(), 0);

        ID3D11RasterizerState* rasterizer = m_Rasterizer.Get();
        if (surface == SurfaceMode::Wireframe) rasterizer = m_WireframeRasterizer.Get();
        else if (surface == SurfaceMode::DepthOnly) rasterizer = m_DepthOnlyRasterizer.Get();
        context->RSSetState(rasterizer);

        ID3D11InputLayout* inputLayout = m_InputLayout.Get();
        ID3D11VertexShader* vertexShader = m_VertexShader.Get();

        switch (layout)
        {
        case VertexLayout::SphereInstanced:
            inputLayout = m_InstancedInputLayout.Get();
            vertexShader = m_InstancedVertexShader.Get();
            break;

        case VertexLayout::MeshInstanced:
            inputLayout = m_MeshInputLayout.Get();
            vertexShader = m_MeshVertexShader.Get();
            break;

        case VertexLayout::Colored:
        default:
            break;
        }

        context->IASetInputLayout(inputLayout);

        ID3D11Buffer* cameraBuffer = m_CameraBuffer.Get();
        context->VSSetShader(vertexShader, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &cameraBuffer);
        context->PSSetShader(m_PixelShader.Get(), nullptr, 0);

        context->HSSetShader(nullptr, nullptr, 0);
        context->DSSetShader(nullptr, nullptr, 0);
        context->GSSetShader(nullptr, nullptr, 0);
    }

    auto GpuPipeline::Release() -> void
    {
        m_MeshInputLayout.Reset();
        m_MeshVertexShader.Reset();
        m_CameraBuffer.Reset();
        m_MaskBuffer.Reset();
        m_InstancedInputLayout.Reset();
        m_InstancedVertexShader.Reset();
        m_InputLayout.Reset();
        m_NoColorBlendState.Reset();
        m_TranslucentBlendState.Reset();
        m_BlendState.Reset();
        m_OverlayDepthState.Reset();
        m_DepthState.Reset();
        m_WireframeRasterizer.Reset();
        m_DepthOnlyRasterizer.Reset();
        m_Rasterizer.Reset();
        m_PixelShader.Reset();
        m_VertexShader.Reset();

        m_IsReady = false;
    }
}