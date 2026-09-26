module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :RaycastPass;

import Common.Math.Type;
import Platform.Render.System;
import Egocentric.Raycast.Type;
import Viewer.Render.Common;
import std;

namespace
{
	using Vertex = Platform::Render::System::GpuPipeline::Vertex;
	using Vec3 = Common::Math::Type::Vec3;
	using RaycastHit = Egocentric::Raycast::Type::RaycastHit;
	using HitKind = Egocentric::Raycast::Type::HitKind;

	constexpr UINT k_InitialCapacity = 64;

	struct Color { float R{}, G{}, B{}; };

	constexpr Color k_AimHitDynamic{ 1.0f, 0.0f, 0.0f };
	constexpr Color k_AimHitStatic{ 0.0f, 1.0f, 0.0f };
	constexpr Color k_AimNoHit{ 1.0f, 1.0f, 1.0f };

	constexpr Color k_PerceptionHitDynamic{ 1.0f, 0.0f, 1.0f };
	constexpr Color k_PerceptionHitStatic{ 1.0f, 1.0f, 0.0f };
	constexpr Color k_PerceptionNoHit{ 1.0f, 1.0f, 1.0f };

	auto EndpointOf(const RaycastHit& hit) -> Vec3
	{
		if (hit.Hit) return hit.Point;

		return {
			hit.Origin.X + hit.Direction.X * hit.MaxDistance,
			hit.Origin.Y + hit.Direction.Y * hit.MaxDistance,
			hit.Origin.Z + hit.Direction.Z * hit.MaxDistance
		};
	}

	auto ColorFor(const RaycastHit& hit, bool isAimRay) -> Color
	{
		if (!hit.Hit) return isAimRay ? k_AimNoHit : k_PerceptionNoHit;

		const bool isDynamic = hit.Kind == HitKind::Dynamic;

		if (isAimRay) return isDynamic ? k_AimHitDynamic : k_AimHitStatic;

		return isDynamic ? k_PerceptionHitDynamic : k_PerceptionHitStatic;
	}

	auto AppendRay(std::vector<Vertex>& vertices, const RaycastHit& hit, bool isAimRay) -> void
	{
		const Vec3 end = EndpointOf(hit);
		const Color color = ColorFor(hit, isAimRay);

		vertices.push_back(Vertex{ hit.Origin.X, hit.Origin.Y, hit.Origin.Z, color.R, color.G, color.B });
		vertices.push_back(Vertex{ end.X, end.Y, end.Z, color.R, color.G, color.B });
	}
}

namespace Viewer::Map::System
{
	auto RaycastPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
		const std::shared_ptr<const Raycasts>& raycasts, std::uint64_t generation) -> void
	{
		if (!device || !context) return;

		if (m_HasGeneration && generation == m_LastGeneration) return;

		m_LastGeneration = generation;
		m_HasGeneration = true;
		m_VertexCount = 0;

		if (!raycasts) return;

		std::vector<Vertex> vertices;
		vertices.reserve(2 + raycasts->PerceptionHits.size() * 2);

		AppendRay(vertices, raycasts->AimHit, true);

		for (const auto& ray : raycasts->PerceptionHits)
		{
			AppendRay(vertices, ray, false);
		}

		if (vertices.empty()) return;

		const UINT needed = static_cast<UINT>(vertices.size());

		if (!Viewer::Render::Common::GrowDynamicVertexBuffer(device, needed,
			k_InitialCapacity, m_VertexBuffer, m_Capacity, "[RaycastPass]", m_LogsService))
		{
			return;
		}

		if (!Viewer::Render::Common::UploadDynamicVertices(context, m_VertexBuffer.Get(),
			vertices, "[RaycastPass]", m_LogsService))
		{
			return;
		}

		m_VertexCount = needed;
	}

	auto RaycastPass::Draw(ID3D11DeviceContext* context) -> void
	{
		if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

		Viewer::Render::Common::DrawVertexBuffer(context, m_VertexBuffer.Get(),
			m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	}

	auto RaycastPass::Release() -> void
	{
		m_VertexBuffer.Reset();
		m_Capacity = 0;
		m_VertexCount = 0;

		m_LastGeneration = 0;
		m_HasGeneration = false;
	}
}