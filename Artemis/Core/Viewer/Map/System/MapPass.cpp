module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :MapPass;

import Platform.Render.System;
import Viewer.Render.Common;
import std;

namespace
{
	using Vertex = Platform::Render::System::GpuPipeline::Vertex;

	constexpr float k_MapRed = 0.34f;
	constexpr float k_MapGreen = 0.52f;
	constexpr float k_MapBlue = 0.64f;
}

namespace Viewer::Map::System
{
	auto MapPass::Upload(ID3D11Device* device, const std::vector<Sbsp>& sbsps) -> void
	{
		if (m_IsUploaded || !device) return;

		m_IsUploaded = true;

		std::size_t triangleCount = 0;
		for (const auto& sbsp : sbsps) triangleCount += sbsp.RenderGeometry.size();

		if (triangleCount == 0)
		{
			m_LogsService.Message("[MapPass] WARNING: The map has no render geometry.");
			return;
		}

		std::vector<Vertex> vertices;
		vertices.reserve(triangleCount * 3);

		auto push = [&vertices](const auto& point) {
			vertices.push_back(Vertex{ point.X, point.Y, point.Z,
				k_MapRed, k_MapGreen, k_MapBlue });
			};

		for (const auto& sbsp : sbsps)
		{
			for (const auto& triangle : sbsp.RenderGeometry)
			{
				push(triangle.A);
				push(triangle.B);
				push(triangle.C);
			}
		}

		const std::size_t byteWidth = vertices.size() * sizeof(Vertex);
		if (byteWidth > (std::numeric_limits<UINT>::max)())
		{
			m_LogsService.Message("[MapPass] ERROR: The map mesh is too big for one buffer.");
			return;
		}

		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.ByteWidth = static_cast<UINT>(byteWidth);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = vertices.data();

		if (FAILED(device->CreateBuffer(&desc, &data, m_VertexBuffer.GetAddressOf())))
		{
			m_LogsService.Message("[MapPass] ERROR: The map vertex buffer failed.");
			return;
		}

		m_VertexCount = static_cast<UINT>(vertices.size());

		m_LogsService.Message("[MapPass] INFO: Uploaded map mesh,"
			" {} vertices ({} triangles).", m_VertexCount, triangleCount);
	}

	auto MapPass::IsUploaded() const -> bool
	{
		return m_IsUploaded;
	}

	auto MapPass::HasBuffer() const -> bool
	{
		return m_VertexBuffer != nullptr && m_VertexCount > 0;
	}

	auto MapPass::Draw(ID3D11DeviceContext* context) -> void
	{
		if (!context || !this->HasBuffer()) return;

		Viewer::Render::Common::DrawVertexBuffer(context, m_VertexBuffer.Get(),
			m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	auto MapPass::Release() -> void
	{
		m_VertexBuffer.Reset();
		m_VertexCount = 0;
		m_IsUploaded = false;
	}
}