module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :ZonePass;
import :ZoneGeometry;

import Platform.Render.System;
import Common.Math.Type;
import Common.ZoneShape.Type;
import std;

namespace
{
	using Vertex = Platform::Render::System::GpuPipeline::Vertex;
	using Vec3 = Common::Math::Type::Vec3;
	using ZoneShape = Common::ZoneShape::Type::ZoneShape;
	using ZoneKind = Common::ZoneShape::Type::Kind;

	constexpr UINT k_InitialCapacity = 2048;
}

namespace Viewer::Map::System
{
	auto ZonePass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
		const std::shared_ptr<const Tick>& tick, const PaletteService& palette) -> void
	{
		if (!device || !context) return;

		const std::uint64_t generation = tick ? tick->Generation : 0;
		if (m_HasGeneration && tick && generation == m_LastGeneration) return;

		m_LastGeneration = generation;
		m_HasGeneration = true;
		m_VertexCount = 0;

		if (!tick || !tick->Fixtures) return;

		const auto& fixtures = *tick->Fixtures;

		std::vector<Vertex> vertices;

		auto emit = [&](std::uint32_t handle, const Vec3& position,
			const Vec3& forward, const Vec3& up, const ZoneShape& zone) {
				if (zone.Kind == ZoneKind::None) return;

				ZoneGeometry::AppendSolid(vertices, position, forward, up,
					zone, palette.ColorOf(handle));
			};

		for (const auto& teleporter : fixtures.Teleporters)
		{
			emit(teleporter.Handle, teleporter.Position, teleporter.Forward,
				teleporter.Up, teleporter.ZoneShape);
		}

		for (const auto& spawn : fixtures.ObjectiveSpawns)
		{
			emit(spawn.Handle, spawn.Position, spawn.Forward,
				spawn.Up, spawn.ZoneShape);
		}

		if (vertices.empty()) return;

		const UINT needed = static_cast<UINT>(vertices.size());

		if (needed > m_Capacity)
		{
			UINT newCapacity = m_Capacity ? m_Capacity : k_InitialCapacity;
			while (newCapacity < needed) newCapacity *= 2;

			if (!this->EnsureCapacity(device, newCapacity)) return;
		}

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (FAILED(context->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			m_LogsService.Message("[ZonePass] ERROR: Map failed.");
			return;
		}

		std::memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
		context->Unmap(m_VertexBuffer.Get(), 0);

		m_VertexCount = needed;
	}

	auto ZonePass::EnsureCapacity(ID3D11Device* device, UINT vertexCapacity) -> bool
	{
		m_VertexBuffer.Reset();
		m_Capacity = 0;

		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = vertexCapacity * Platform::Render::System::GpuPipeline::k_VertexStride;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		if (FAILED(device->CreateBuffer(&desc, nullptr, m_VertexBuffer.GetAddressOf())))
		{
			m_LogsService.Message("[ZonePass] ERROR: The vertex buffer failed.");
			return false;
		}

		m_Capacity = vertexCapacity;
		return true;
	}

	auto ZonePass::Draw(ID3D11DeviceContext* context) -> void
	{
		if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

		ID3D11Buffer* buffer = m_VertexBuffer.Get();
		const UINT stride = Platform::Render::System::GpuPipeline::k_VertexStride;
		const UINT offset = 0;

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
		context->Draw(m_VertexCount, 0);
	}

	auto ZonePass::Release() -> void
	{
		m_VertexBuffer.Reset();
		m_Capacity = 0;
		m_VertexCount = 0;

		m_LastGeneration = 0;
		m_HasGeneration = false;
	}
}