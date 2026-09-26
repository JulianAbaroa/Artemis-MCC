module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :DynamicPass;

import Platform.Render.System;
import Viewer.Selection.State;
import Viewer.Render.Common;
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

		if (!Viewer::Render::Common::GrowDynamicVertexBuffer(device, needed,
			k_InitialCapacity, m_VertexBuffer, m_Capacity, "[DynamicPass]", m_LogsService))
		{
			m_VertexCount = 0;
			return;
		}

		if (!Viewer::Render::Common::UploadDynamicVertices(context, m_VertexBuffer.Get(),
			vertices, "[DynamicPass]", m_LogsService))
		{
			m_VertexCount = 0;
			return;
		}

		m_VertexCount = needed;
	}

	auto DynamicPass::GetBounds() const -> std::span<const ObjectBounds>
	{
		return m_Bounds;
	}

	auto DynamicPass::Draw(ID3D11DeviceContext* context) -> void
	{
		if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

		Viewer::Render::Common::DrawVertexBuffer(context, m_VertexBuffer.Get(),
			m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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