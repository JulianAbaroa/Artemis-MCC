module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :RaycastPass;

import Platform.Render.Type;
import Platform.Render.System;
import Common.Math.Type;
import Egocentric.Raycast.Type;
import Viewer.Style.Type;
import std;

namespace
{
    using Vertex = Platform::Render::Type::Vertex;
    using Vec3 = Common::Math::Type::Vec3;
    using RaycastHit = Egocentric::Raycast::Type::RaycastHit;
    using HitKind = Egocentric::Raycast::Type::HitKind;
    using Color = Viewer::Style::Type::Color;

    using Platform::Render::Type::VertexLayout;

    using Viewer::Style::Type::k_RayAimNoHit;
    using Viewer::Style::Type::k_RayPerceptionNoHit;
    using Viewer::Style::Type::k_RayAimHitDynamic;
    using Viewer::Style::Type::k_RayAimHitStatic;
    using Viewer::Style::Type::k_RayPerceptionHitDynamic;
    using Viewer::Style::Type::k_RayPerceptionHitStatic;

    constexpr UINT k_InitialCapacity{ 64 };

    // return: The hit point, or the end of the ray at its maximum distance if it hit nothing.
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
        if (!hit.Hit) return isAimRay ? k_RayAimNoHit : k_RayPerceptionNoHit;

        const bool isDynamic = hit.Kind == HitKind::Dynamic;

        if (isAimRay) return isDynamic ? k_RayAimHitDynamic : k_RayAimHitStatic;

        return isDynamic ? k_RayPerceptionHitDynamic : k_RayPerceptionHitStatic;
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

        if (m_Gate.IsCurrent(generation)) return;

        m_Gate.Mark(generation);
        m_VertexCount = 0;

        if (!raycasts) return;

        std::vector<Vertex> vertices{};
        vertices.reserve(2 + raycasts->PerceptionHits.size() * 2);

        AppendRay(vertices, raycasts->AimHit, true);

        for (const auto& ray : raycasts->PerceptionHits)
        {
            AppendRay(vertices, ray, false);
        }

        if (vertices.empty()) return;

        const UINT needed = static_cast<UINT>(vertices.size());

        if (!Platform::Render::System::GpuBuffer::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, m_VertexBuffer, m_Capacity, "[RaycastPass]", m_LogsService))
        {
            return;
        }

        if (!Platform::Render::System::GpuBuffer::UploadDynamicVertices(context, m_VertexBuffer.Get(),
            vertices, "[RaycastPass]", m_LogsService))
        {
            return;
        }

        m_VertexCount = needed;
    }

    auto RaycastPass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

        pipeline.Bind(context, VertexLayout::Colored);

        Platform::Render::System::GpuBuffer::DrawVertexBuffer(context, m_VertexBuffer.Get(),
            m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    auto RaycastPass::Release() -> void
    {
        m_VertexBuffer.Reset();
        m_Capacity = 0;
        m_VertexCount = 0;

        m_Gate.Reset();
    }
}