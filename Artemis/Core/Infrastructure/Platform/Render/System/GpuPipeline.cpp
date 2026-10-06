module;

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#pragma comment(lib, "d3dcompiler.lib")

module Platform.Render.System;
import :GpuPipeline;
import :Shaders;

import std;

namespace
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    using LogsService = Service::Logs::System::LogsService;

    using Platform::Render::System::Shaders::k_MapSource;

    // Compiles an entry point of the map shader source.
    // return: False if it fails. The compiler errors are logged.
    auto CompileShader(const char* entry, const char* target,
        LogsService& logs, ComPtr<ID3DBlob>& blob) -> bool
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

namespace Platform::Render::System
{
    auto GpuPipeline::Initialize(ID3D11Device* device) -> bool
    {
        if (m_IsReady) return true;

        if (!device)
        {
            m_LogsService.Message("[GpuPipeline] ERROR: No device.");
            return false;
        }

        if (!this->CompileShaders(device) || !this->CreateBuffers(device) ||
            !this->CreateStates(device))
        {
            this->Release();
            return false;
        }

        m_IsReady = true;
        m_LogsService.Message("[GpuPipeline] INFO: Ready.");
        return true;
    }

    auto GpuPipeline::CompileShaders(ID3D11Device* device) -> bool
    {
        ComPtr<ID3DBlob> vertexBlob{};
        ComPtr<ID3DBlob> pixelBlob{};
        ComPtr<ID3DBlob> instancedBlob{};
        ComPtr<ID3DBlob> meshBlob{};

        if (!CompileShader("VSInstanced", "vs_5_0", m_LogsService, instancedBlob)) return false;
        if (!CompileShader("VSInstancedMesh", "vs_5_0", m_LogsService, meshBlob)) return false;
        if (!CompileShader("VSMain", "vs_5_0", m_LogsService, vertexBlob)) return false;
        if (!CompileShader("PSMain", "ps_5_0", m_LogsService, pixelBlob)) return false;

        if (FAILED(device->CreateVertexShader(vertexBlob->GetBufferPointer(),
            vertexBlob->GetBufferSize(), nullptr, m_VertexShader.GetAddressOf())))
        {
            m_LogsService.Message("[GpuPipeline] ERROR: CreateVertexShader failed.");
            return false;
        }

        if (FAILED(device->CreatePixelShader(pixelBlob->GetBufferPointer(),
            pixelBlob->GetBufferSize(), nullptr, m_PixelShader.GetAddressOf())))
        {
            m_LogsService.Message("[GpuPipeline] ERROR: CreatePixelShader failed.");
            return false;
        }

        if (FAILED(device->CreateVertexShader(instancedBlob->GetBufferPointer(),
            instancedBlob->GetBufferSize(), nullptr, m_InstancedVertexShader.GetAddressOf())))
        {
            m_LogsService.Message("[GpuPipeline] ERROR: CreateVertexShader (instanced) failed.");
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
            m_LogsService.Message("[GpuPipeline] ERROR: CreateInputLayout (instanced) failed.");
            return false;
        }

        if (FAILED(device->CreateVertexShader(meshBlob->GetBufferPointer(),
            meshBlob->GetBufferSize(), nullptr, m_MeshVertexShader.GetAddressOf())))
        {
            m_LogsService.Message("[GpuPipeline] ERROR: CreateVertexShader (mesh) failed.");
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
            m_LogsService.Message("[GpuPipeline] ERROR: CreateInputLayout (mesh) failed.");
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
            m_LogsService.Message("[GpuPipeline] ERROR: CreateInputLayout failed.");
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
            m_LogsService.Message("[GpuPipeline] ERROR: Camera constant buffer failed.");
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
            m_LogsService.Message("[GpuPipeline] ERROR: Rasterizer state failed.");
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC depth{};
        depth.DepthEnable = TRUE;
        depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        depth.StencilEnable = FALSE;

        if (FAILED(device->CreateDepthStencilState(&depth, m_DepthState.GetAddressOf())))
        {
            m_LogsService.Message("[GpuPipeline] ERROR: Depth state failed.");
            return false;
        }

        D3D11_BLEND_DESC blend{};
        blend.RenderTarget[0].BlendEnable = FALSE;
        blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(device->CreateBlendState(&blend, m_BlendState.GetAddressOf())))
        {
            m_LogsService.Message("[GpuPipeline] ERROR: Blend state failed.");
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC overlay{ depth };
        overlay.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        if (FAILED(device->CreateDepthStencilState(&overlay, m_OverlayDepthState.GetAddressOf())))
        {
            m_LogsService.Message("[GpuPipeline] ERROR: Overlay depth state failed.");
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
            m_LogsService.Message("[GpuPipeline] ERROR: Translucent blend state failed.");
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

    auto GpuPipeline::Bind(ID3D11DeviceContext* context, VertexLayout layout,
        SurfaceMode surface, float alpha) -> void
    {
        if (!context || !m_IsReady) return;

        const bool isTranslucent = surface == SurfaceMode::Translucent;
        const float blendFactor[4]{ alpha, alpha, alpha, alpha };

        context->OMSetBlendState(
            isTranslucent ? m_TranslucentBlendState.Get() : m_BlendState.Get(),
            blendFactor, 0xffffffff);
        context->OMSetDepthStencilState(
            surface == SurfaceMode::Opaque ? m_DepthState.Get() : m_OverlayDepthState.Get(), 0);
        context->RSSetState(m_Rasterizer.Get());

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
        m_InstancedInputLayout.Reset();
        m_InstancedVertexShader.Reset();
        m_InputLayout.Reset();
        m_TranslucentBlendState.Reset();
        m_BlendState.Reset();
        m_OverlayDepthState.Reset();
        m_DepthState.Reset();
        m_Rasterizer.Reset();
        m_PixelShader.Reset();
        m_VertexShader.Reset();

        m_IsReady = false;
    }
}