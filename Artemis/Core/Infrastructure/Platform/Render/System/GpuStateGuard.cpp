module;

#include <d3d11.h>
#include <wrl/client.h>

module Platform.Render.System;
import :GpuStateGuard;

import std;

namespace Platform::Render::System
{
	GpuStateGuard::GpuStateGuard(ID3D11DeviceContext* context) : m_Context(context)
	{
		if (!m_Context) return;

		m_Context->IAGetInputLayout(m_InputLayout.GetAddressOf());
		m_Context->IAGetPrimitiveTopology(&m_Topology);
		m_Context->IAGetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(),
			&m_VertexStride, &m_VertexOffset);
		m_Context->IAGetIndexBuffer(m_IndexBuffer.GetAddressOf(),
			&m_IndexFormat, &m_IndexOffset);

		m_Context->VSGetShader(m_VertexShader.GetAddressOf(), nullptr, nullptr);
		m_Context->VSGetConstantBuffers(0, 1, m_VertexConstantBuffer.GetAddressOf());
		m_Context->HSGetShader(m_HullShader.GetAddressOf(), nullptr, nullptr);
		m_Context->DSGetShader(m_DomainShader.GetAddressOf(), nullptr, nullptr);
		m_Context->GSGetShader(m_GeometryShader.GetAddressOf(), nullptr, nullptr);
		m_Context->PSGetShader(m_PixelShader.GetAddressOf(), nullptr, nullptr);

		m_Context->RSGetState(m_Rasterizer.GetAddressOf());
		m_Context->RSGetViewports(&m_ViewportCount, m_Viewports.data());

		m_Context->OMGetRenderTargets(1, m_RenderTarget.GetAddressOf(),
			m_DepthStencil.GetAddressOf());
		m_Context->OMGetDepthStencilState(m_DepthState.GetAddressOf(), &m_StencilRef);
		m_Context->OMGetBlendState(m_BlendState.GetAddressOf(),
			m_BlendFactor.data(), &m_SampleMask);
	}

	GpuStateGuard::~GpuStateGuard()
	{
		if (!m_Context) return;

		m_Context->IASetInputLayout(m_InputLayout.Get());
		m_Context->IASetPrimitiveTopology(m_Topology);

		ID3D11Buffer* vertexBuffer = m_VertexBuffer.Get();
		m_Context->IASetVertexBuffers(0, 1, &vertexBuffer,
			&m_VertexStride, &m_VertexOffset);
		m_Context->IASetIndexBuffer(m_IndexBuffer.Get(), m_IndexFormat, m_IndexOffset);

		ID3D11Buffer* vertexConstantBuffer = m_VertexConstantBuffer.Get();
		m_Context->VSSetShader(m_VertexShader.Get(), nullptr, 0);
		m_Context->VSSetConstantBuffers(0, 1, &vertexConstantBuffer);
		m_Context->HSSetShader(m_HullShader.Get(), nullptr, 0);
		m_Context->DSSetShader(m_DomainShader.Get(), nullptr, 0);
		m_Context->GSSetShader(m_GeometryShader.Get(), nullptr, 0);
		m_Context->PSSetShader(m_PixelShader.Get(), nullptr, 0);

		m_Context->RSSetState(m_Rasterizer.Get());
		m_Context->RSSetViewports(m_ViewportCount,
			m_ViewportCount > 0 ? m_Viewports.data() : nullptr);

		ID3D11RenderTargetView* renderTarget = m_RenderTarget.Get();
		m_Context->OMSetRenderTargets(1, &renderTarget, m_DepthStencil.Get());
		m_Context->OMSetDepthStencilState(m_DepthState.Get(), m_StencilRef);
		m_Context->OMSetBlendState(m_BlendState.Get(), m_BlendFactor.data(), m_SampleMask);
	}
}