module;

#include <d3d11.h>
#include <wrl/client.h>

module Engine.Render.System;
import :GpuStateGuard;
import Engine.Render.Type;
import std;

namespace
{
    using Engine::Render::Type::k_MaskBufferSlot;
}

namespace Engine::Render::System
{
    GpuStateGuard::GpuStateGuard(ID3D11DeviceContext* context) : m_Context(context)
    {
        if (!m_Context) return;

        m_Context->IAGetInputLayout(m_InputLayout.GetAddressOf());
        m_Context->IAGetPrimitiveTopology(&m_Topology);

        ID3D11Buffer* vertexBuffers[k_VertexSlots]{};
        m_Context->IAGetVertexBuffers(0, k_VertexSlots, vertexBuffers,
            m_VertexStrides.data(), m_VertexOffsets.data());
        for (UINT i = 0; i < k_VertexSlots; ++i) m_VertexBuffers[i].Attach(vertexBuffers[i]);

        m_Context->IAGetIndexBuffer(m_IndexBuffer.GetAddressOf(),
            &m_IndexFormat, &m_IndexOffset);

        m_Context->VSGetShader(m_VertexShader.GetAddressOf(), nullptr, nullptr);
        m_Context->VSGetConstantBuffers(0, 1, m_VertexConstantBuffer.GetAddressOf());
        m_Context->HSGetShader(m_HullShader.GetAddressOf(), nullptr, nullptr);
        m_Context->DSGetShader(m_DomainShader.GetAddressOf(), nullptr, nullptr);
        m_Context->GSGetShader(m_GeometryShader.GetAddressOf(), nullptr, nullptr);
        m_Context->PSGetShader(m_PixelShader.GetAddressOf(), nullptr, nullptr);
        m_Context->PSGetConstantBuffers(k_MaskBufferSlot, 1, m_PixelConstantBuffer.GetAddressOf());

        ID3D11ShaderResourceView* pixelResources[k_PixelResourceSlots]{};
        m_Context->PSGetShaderResources(0, k_PixelResourceSlots, pixelResources);
        for (UINT i = 0; i < k_PixelResourceSlots; ++i) m_PixelResources[i].Attach(pixelResources[i]);

        m_Context->RSGetState(m_Rasterizer.GetAddressOf());
        m_Context->GetPredication(m_Predicate.GetAddressOf(), &m_PredicateValue);
        m_Context->SetPredication(nullptr, FALSE);

        // The counts are asked first, so exactly the bound ones are saved and put back.
        m_Context->RSGetViewports(&m_ViewportCount, nullptr);
        m_ViewportCount = (std::min)(m_ViewportCount, static_cast<UINT>(m_Viewports.size()));
        m_Context->RSGetViewports(&m_ViewportCount, m_Viewports.data());

        m_Context->RSGetScissorRects(&m_ScissorCount, nullptr);
        m_ScissorCount = (std::min)(m_ScissorCount, static_cast<UINT>(m_Scissors.size()));
        m_Context->RSGetScissorRects(&m_ScissorCount, m_Scissors.data());

        ID3D11RenderTargetView* renderTargets[k_TargetSlots]{};
        m_Context->OMGetRenderTargets(k_TargetSlots, renderTargets, m_DepthStencil.GetAddressOf());
        for (UINT i = 0; i < k_TargetSlots; ++i) m_RenderTargets[i].Attach(renderTargets[i]);
        m_Context->OMGetDepthStencilState(m_DepthState.GetAddressOf(), &m_StencilRef);
        m_Context->OMGetBlendState(m_BlendState.GetAddressOf(),
            m_BlendFactor.data(), &m_SampleMask);
    }

    GpuStateGuard::~GpuStateGuard()
    {
        if (!m_Context) return;

        m_Context->IASetInputLayout(m_InputLayout.Get());
        m_Context->IASetPrimitiveTopology(m_Topology);

        ID3D11Buffer* vertexBuffers[k_VertexSlots]{};
        for (UINT i = 0; i < k_VertexSlots; ++i) vertexBuffers[i] = m_VertexBuffers[i].Get();
        m_Context->IASetVertexBuffers(0, k_VertexSlots, vertexBuffers,
            m_VertexStrides.data(), m_VertexOffsets.data());
        m_Context->IASetIndexBuffer(m_IndexBuffer.Get(), m_IndexFormat, m_IndexOffset);

        ID3D11Buffer* vertexConstantBuffer = m_VertexConstantBuffer.Get();
        m_Context->VSSetShader(m_VertexShader.Get(), nullptr, 0);
        m_Context->VSSetConstantBuffers(0, 1, &vertexConstantBuffer);
        m_Context->HSSetShader(m_HullShader.Get(), nullptr, 0);
        m_Context->DSSetShader(m_DomainShader.Get(), nullptr, 0);
        m_Context->GSSetShader(m_GeometryShader.Get(), nullptr, 0);
        m_Context->PSSetShader(m_PixelShader.Get(), nullptr, 0);

        ID3D11Buffer* pixelConstantBuffer = m_PixelConstantBuffer.Get();
        m_Context->PSSetConstantBuffers(k_MaskBufferSlot, 1, &pixelConstantBuffer);

        ID3D11ShaderResourceView* pixelResources[k_PixelResourceSlots]{};
        for (UINT i = 0; i < k_PixelResourceSlots; ++i) pixelResources[i] = m_PixelResources[i].Get();
        m_Context->PSSetShaderResources(0, k_PixelResourceSlots, pixelResources);

        m_Context->RSSetState(m_Rasterizer.Get());
        m_Context->RSSetViewports(m_ViewportCount,
            m_ViewportCount > 0 ? m_Viewports.data() : nullptr);

        m_Context->RSSetScissorRects(m_ScissorCount, m_ScissorCount > 0 ? m_Scissors.data() : nullptr);

        ID3D11RenderTargetView* renderTargets[k_TargetSlots]{};
        UINT targetCount{ 0 };
        for (UINT i = 0; i < k_TargetSlots; ++i)
        {
            renderTargets[i] = m_RenderTargets[i].Get();
            if (renderTargets[i] != nullptr) targetCount = i + 1;
        }
        m_Context->OMSetRenderTargets(targetCount, renderTargets, m_DepthStencil.Get());
        m_Context->SetPredication(m_Predicate.Get(), m_PredicateValue);
        m_Context->OMSetDepthStencilState(m_DepthState.Get(), m_StencilRef);
        m_Context->OMSetBlendState(m_BlendState.Get(), m_BlendFactor.data(), m_SampleMask);
    }
}