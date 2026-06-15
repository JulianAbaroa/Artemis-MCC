#include "pch.h"

#include "GpuStateGuard.h"

namespace
{
    template <typename T>
    inline void SafeRelease(T*& p)
    {
        if (p) { p->Release(); p = nullptr; }
    }
}

GpuStateGuard::GpuStateGuard(ID3D11DeviceContext* ctx)
    : m_Ctx(ctx)
{
    if (!m_Ctx) return;

    m_Ctx->IAGetInputLayout(&m_InputLayout);
    m_Ctx->IAGetPrimitiveTopology(&m_Topology);
    m_Ctx->IAGetVertexBuffers(0, 1, &m_VertexBuffer, &m_VertexStride, &m_VertexOffset);
    m_Ctx->IAGetIndexBuffer(&m_IndexBuffer, &m_IndexFormat, &m_IndexOffset);

    m_Ctx->VSGetShader(&m_VS, nullptr, nullptr);
    m_Ctx->VSGetConstantBuffers(0, 1, &m_VSConstantBuffer);

    m_Ctx->PSGetShader(&m_PS, nullptr, nullptr);
    m_Ctx->PSGetConstantBuffers(0, 1, &m_PSConstantBuffer);

    m_Ctx->RSGetState(&m_RasterState);
    m_Ctx->RSGetViewports(&m_NumViewports, m_Viewports);

    m_Ctx->OMGetRenderTargets(1, &m_RTV, &m_DSV);
    m_Ctx->OMGetDepthStencilState(&m_DepthStencilState, &m_StencilRef);
    m_Ctx->OMGetBlendState(&m_BlendState, m_BlendFactor, &m_SampleMask);
}

void GpuStateGuard::Restore()
{
    if (!m_Ctx || m_Restored) return;

    m_Ctx->IASetInputLayout(m_InputLayout);
    m_Ctx->IASetPrimitiveTopology(m_Topology);
    m_Ctx->IASetVertexBuffers(0, 1, &m_VertexBuffer, &m_VertexStride, &m_VertexOffset);
    m_Ctx->IASetIndexBuffer(m_IndexBuffer, m_IndexFormat, m_IndexOffset);

    m_Ctx->VSSetShader(m_VS, nullptr, 0);
    m_Ctx->VSSetConstantBuffers(0, 1, &m_VSConstantBuffer);

    m_Ctx->PSSetShader(m_PS, nullptr, 0);
    m_Ctx->PSSetConstantBuffers(0, 1, &m_PSConstantBuffer);

    m_Ctx->RSSetState(m_RasterState);
    m_Ctx->RSSetViewports(m_NumViewports, m_Viewports);

    m_Ctx->OMSetRenderTargets(1, &m_RTV, m_DSV);
    m_Ctx->OMSetDepthStencilState(m_DepthStencilState, m_StencilRef);
    m_Ctx->OMSetBlendState(m_BlendState, m_BlendFactor, m_SampleMask);

    m_Restored = true;
}

GpuStateGuard::~GpuStateGuard()
{
    SafeRelease(m_InputLayout);
    SafeRelease(m_VertexBuffer);
    SafeRelease(m_IndexBuffer);

    SafeRelease(m_VS);
    SafeRelease(m_VSConstantBuffer);

    SafeRelease(m_PS);
    SafeRelease(m_PSConstantBuffer);

    SafeRelease(m_RasterState);

    SafeRelease(m_RTV);
    SafeRelease(m_DSV);
    SafeRelease(m_DepthStencilState);
    SafeRelease(m_BlendState);
}