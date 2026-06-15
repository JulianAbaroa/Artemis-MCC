#include "pch.h"

#include "GpuPipeline.h"

#include "Shaders.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <d3dcompiler.h>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    constexpr float kFillAlpha = 0.50f;
}

bool GpuPipeline::Init(ID3D11Device* device, System_Logs& logs)
{
    if (m_Ready) return true;

    if (!device)
    {
        logs.Log("[GpuPipeline] ERROR: no device.");
        return false;
    }

    if (!CompileShaders(device, logs)) { Release(); return false; }
    if (!CreateStates(device, logs)) { Release(); return false; }

    m_Ready = true;
    logs.Log("[GpuPipeline] INFO: ready.");
    return true;
}

bool GpuPipeline::CompileShaders(ID3D11Device* device, System_Logs& logs)
{
    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    const char* src = Shaders::kMapSource;
    const SIZE_T srcLen = std::strlen(src);

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errors = nullptr;

    HRESULT hr = D3DCompile(src, srcLen, nullptr, nullptr, nullptr,
        "VSMain", "vs_5_0", flags, 0, &vsBlob, &errors);
    if (FAILED(hr))
    {
        if (errors)
        {
            logs.Log("[GpuPipeline] VS compile error: %s",
                static_cast<const char*>(errors->GetBufferPointer()));
            errors->Release();
        }
        if (vsBlob) vsBlob->Release();
        return false;
    }
    if (errors) { errors->Release(); errors = nullptr; }

    hr = D3DCompile(src, srcLen, nullptr, nullptr, nullptr,
        "PSMain", "ps_5_0", flags, 0, &psBlob, &errors);
    if (FAILED(hr))
    {
        if (errors)
        {
            logs.Log("[GpuPipeline] PS compile error: %s",
                static_cast<const char*>(errors->GetBufferPointer()));
            errors->Release();
        }
        if (psBlob) psBlob->Release();
        if (vsBlob) vsBlob->Release();
        return false;
    }
    if (errors) { errors->Release(); errors = nullptr; }

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), nullptr, &m_VS);
    if (FAILED(hr)) { vsBlob->Release(); psBlob->Release(); return false; }

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(), nullptr, &m_PS);
    if (FAILED(hr)) { vsBlob->Release(); psBlob->Release(); return false; }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(layout, 2,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_InputLayout);

    vsBlob->Release();
    psBlob->Release();

    if (FAILED(hr))
    {
        logs.Log("[GpuPipeline] ERROR: CreateInputLayout failed.");
        return false;
    }

    D3D11_BUFFER_DESC camDesc = {};
    camDesc.ByteWidth = sizeof(float) * 16;
    camDesc.Usage = D3D11_USAGE_DYNAMIC;
    camDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    camDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&camDesc, nullptr, &m_CameraCB)))
    {
        logs.Log("[GpuPipeline] ERROR: camera cbuffer failed.");
        return false;
    }

    D3D11_BUFFER_DESC tintDesc = {};
    tintDesc.ByteWidth = sizeof(float) * 4;
    tintDesc.Usage = D3D11_USAGE_DYNAMIC;
    tintDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    tintDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&tintDesc, nullptr, &m_TintCB)))
    {
        logs.Log("[GpuPipeline] ERROR: tint cbuffer failed.");
        return false;
    }

    return true;
}

bool GpuPipeline::CreateStates(ID3D11Device* device, System_Logs& logs)
{
    D3D11_RASTERIZER_DESC rs = {};
    rs.CullMode = D3D11_CULL_NONE;
    rs.FrontCounterClockwise = FALSE;
    rs.DepthClipEnable = TRUE;

    rs.FillMode = D3D11_FILL_SOLID;
    if (FAILED(device->CreateRasterizerState(&rs, &m_RasterSolid)))
    {
        logs.Log("[GpuPipeline] ERROR: solid rasterizer failed.");
        return false;
    }

    rs.FillMode = D3D11_FILL_WIREFRAME;
    rs.DepthBias = 0;
    rs.SlopeScaledDepthBias = 0.0f;
    rs.DepthBiasClamp = 0.0f;
    if (FAILED(device->CreateRasterizerState(&rs, &m_RasterWire)))
    {
        logs.Log("[GpuPipeline] ERROR: wire rasterizer failed.");
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC ds = {};
    ds.DepthEnable = TRUE;
    ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    ds.StencilEnable = FALSE;

    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    if (FAILED(device->CreateDepthStencilState(&ds, &m_DepthWrite)))
    {
        logs.Log("[GpuPipeline] ERROR: depth-write state failed.");
        return false;
    }

    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    if (FAILED(device->CreateDepthStencilState(&ds, &m_DepthNoWrite)))
    {
        logs.Log("[GpuPipeline] ERROR: depth-no-write state failed.");
        return false;
    }

    D3D11_BLEND_DESC bs = {};
    bs.AlphaToCoverageEnable = FALSE;
    bs.IndependentBlendEnable = FALSE;
    bs.RenderTarget[0].BlendEnable = TRUE;
    bs.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bs.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bs.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bs.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bs.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bs.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bs.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    if (FAILED(device->CreateBlendState(&bs, &m_BlendState)))
    {
        logs.Log("[GpuPipeline] ERROR: blend state failed.");
        return false;
    }

    D3D11_BLEND_DESC bsNoColor = {};
    bsNoColor.RenderTarget[0].BlendEnable = FALSE;
    bsNoColor.RenderTarget[0].RenderTargetWriteMask = 0;
    if (FAILED(device->CreateBlendState(&bsNoColor, &m_BlendNoColor)))
    {
        logs.Log("[GpuPipeline] ERROR: no-color blend state failed.");
        return false;
    }

    return true;
}

void GpuPipeline::UpdateCamera(ID3D11DeviceContext* ctx, const float* viewProj16)
{
    if (!ctx || !m_CameraCB || !viewProj16) return;

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(m_CameraCB, 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, viewProj16, sizeof(float) * 16);
        ctx->Unmap(m_CameraCB, 0);
    }
}

void GpuPipeline::UpdateTint(ID3D11DeviceContext* ctx, Fill fill)
{
    if (!ctx || !m_TintCB) return;

    const float alpha = (fill == Fill::Solid) ? kFillAlpha : (fill == Fill::Wire) ? 1.0f : 1.0f;
    const float data[4] = { alpha, 0.0f, 0.0f, 0.0f };

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(m_TintCB, 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, data, sizeof(data));
        ctx->Unmap(m_TintCB, 0);
    }
}

void GpuPipeline::Bind(ID3D11DeviceContext* ctx, Fill fill)
{
    if (!ctx || !m_Ready) return;

    UpdateTint(ctx, fill);

    const float bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ID3D11BlendState* blend =
        (fill == Fill::DepthPrime) ? m_BlendNoColor : m_BlendState;
    ctx->OMSetBlendState(blend, bf, 0xffffffff);

    ID3D11DepthStencilState* depth =
        (fill == Fill::DepthPrime) ? m_DepthWrite : m_DepthNoWrite;
    ctx->OMSetDepthStencilState(depth, 0);

    ID3D11RasterizerState* raster =
        (fill == Fill::Wire) ? m_RasterWire : m_RasterSolid;
    ctx->RSSetState(raster);

    ctx->IASetInputLayout(m_InputLayout);

    ctx->VSSetShader(m_VS, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &m_CameraCB);

    ctx->PSSetShader(m_PS, nullptr, 0);
    ctx->PSSetConstantBuffers(1, 1, &m_TintCB);
}

void GpuPipeline::Release()
{
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };

    rel(m_TintCB);
    rel(m_CameraCB);
    rel(m_InputLayout);
    rel(m_BlendState);
    rel(m_BlendNoColor);
    rel(m_DepthNoWrite);
    rel(m_DepthWrite);
    rel(m_RasterWire);
    rel(m_RasterSolid);
    rel(m_PS);
    rel(m_VS);

    m_Ready = false;
}