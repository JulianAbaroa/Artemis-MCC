module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :DynamicPass;

import Viewer.Selection.State;
import Platform.Render.System;
import std;

namespace
{
	using Vertex = Platform::Render::System::GpuPipeline::Vertex;
	using Color = Viewer::Map::Type::Color;

	constexpr Color k_SelectedColor = { 1.0f, 1.0f, 1.0f };
	constexpr UINT k_InitialCapacity = 4096;
}

namespace Viewer::Map::System
{
	auto DynamicPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
		const std::shared_ptr<const Collidables>& collidables,
		const PaletteService& palette, std::uint32_t selectedHandle,
		std::uint64_t generation) -> void
	{
		using Viewer::Selection::State::k_NoSelection;

		if (!device || !context) return;

		const bool isSameTick = m_HasGeneration && generation == m_LastGeneration;
		const bool isSameSelection = selectedHandle == m_LastSelected;
		if (isSameTick && isSameSelection) return;

		m_LastGeneration = generation;
		m_HasGeneration = true;
		m_LastSelected = selectedHandle;

		m_Bounds.clear();

		if (!collidables || collidables->empty())
		{
			m_VertexCount = 0;
			return;
		}

		std::size_t triangleCount = 0;
		for (const auto& collidable : *collidables)
		{
			triangleCount += collidable.WorldMesh.Triangles.size();
		}

		std::vector<Vertex> vertices;
		vertices.reserve(triangleCount * 3);
		m_Bounds.reserve(collidables->size());

		for (const auto& collidable : *collidables)
		{
			const auto& triangles = collidable.WorldMesh.Triangles;
			if (triangles.empty()) continue;

			const bool isSelected = selectedHandle != k_NoSelection &&
				collidable.Handle == selectedHandle;

			const Color color = isSelected ? k_SelectedColor : palette.ColorOf(collidable.Handle);

			ObjectBounds bounds;
			bounds.Handle = collidable.Handle;
			bounds.Min = triangles[0].A;
			bounds.Max = triangles[0].A;

			auto add = [&](const auto& point) {
				vertices.push_back(Vertex{ point.X, point.Y, point.Z, color.R, color.G, color.B });

				bounds.Min.X = (std::min)(bounds.Min.X, point.X);
				bounds.Min.Y = (std::min)(bounds.Min.Y, point.Y);
				bounds.Min.Z = (std::min)(bounds.Min.Z, point.Z);
				bounds.Max.X = (std::max)(bounds.Max.X, point.X);
				bounds.Max.Y = (std::max)(bounds.Max.Y, point.Y);
				bounds.Max.Z = (std::max)(bounds.Max.Z, point.Z);
				};

			for (const auto& triangle : triangles)
			{
				add(triangle.A);
				add(triangle.B);
				add(triangle.C);
			}

			m_Bounds.push_back(bounds);
		}

		if (vertices.empty())
		{
			m_VertexCount = 0;
			return;
		}

		const UINT needed = static_cast<UINT>(vertices.size());

		if (needed > m_Capacity)
		{
			UINT newCapacity = m_Capacity ? m_Capacity : k_InitialCapacity;
			while (newCapacity < needed) newCapacity *= 2;

			if (!this->EnsureCapacity(device, newCapacity))
			{
				m_VertexCount = 0;
				return;
			}
		}

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (FAILED(context->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			m_LogsService.Message("[DynamicPass] ERROR: Map failed.");
			m_VertexCount = 0;
			return;
		}

		std::memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
		context->Unmap(m_VertexBuffer.Get(), 0);

		m_VertexCount = needed;
	}

	auto DynamicPass::GetBounds() const -> std::span<const ObjectBounds>
	{
		return m_Bounds;
	}

	auto DynamicPass::EnsureCapacity(ID3D11Device* device, UINT vertexCapacity) -> bool
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
			m_LogsService.Message("[DynamicPass] ERROR: The dynamic vertex buffer failed.");
			return false;
		}

		m_Capacity = vertexCapacity;
		return true;
	}

	auto DynamicPass::Draw(ID3D11DeviceContext* context) -> void
	{
		if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

		ID3D11Buffer* buffer = m_VertexBuffer.Get();
		const UINT stride = Platform::Render::System::GpuPipeline::k_VertexStride;
		const UINT offset = 0;

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
		context->Draw(m_VertexCount, 0);
	}

	auto DynamicPass::Release() -> void
	{
		m_VertexBuffer.Reset();
		m_Capacity = 0;
		m_VertexCount = 0;

		m_LastGeneration = 0;
		m_HasGeneration = false;
		m_LastSelected = 0xFFFFFFFF;

		m_Bounds.clear();
	}
}