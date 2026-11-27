module;

#include <d3d11.h>

module DevTools.Viewer.Map.System;
import :RaycastPass;

import Common.Math.Type;
import Engine.Render.Type;
import Engine.Render.System;
import Views.Raycast.Type;
import Views.View.Type;
import Views.View.System;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using Engine::Render::Type::Vertex;
    using Common::Math::Type::Vec3;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::Raycasts;
    using Views::View::Type::View;
    using Views::View::System::RaycastsOf;
    using Views::Raycast::Type::HitKind;
    using DevTools::Viewer::Style::Type::Color;

    using Engine::Render::Type::VertexLayout;

    using DevTools::Viewer::Map::Type::ShownRowsKeyOf;
    using DevTools::Viewer::Style::Type::k_RayAimNoHit;
    using DevTools::Viewer::Style::Type::k_RayPerceptionNoHit;
    using DevTools::Viewer::Style::Type::k_RayAimHitDynamic;
    using DevTools::Viewer::Style::Type::k_RayAimHitStatic;
    using DevTools::Viewer::Style::Type::k_RayPerceptionHitDynamic;
    using DevTools::Viewer::Style::Type::k_RayPerceptionHitStatic;

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

    auto AppendRay(std::vector<Vertex>& vertices, const RaycastHit& hit, bool isAimRay, const Vec3& originOffset) -> void
    {
        const Vec3 end = EndpointOf(hit);
        const Color color = ColorFor(hit, isAimRay);

        vertices.push_back(Vertex{ hit.Origin.X + originOffset.X, hit.Origin.Y + originOffset.Y, hit.Origin.Z + originOffset.Z,
            color.R, color.G, color.B });
        vertices.push_back(Vertex{ end.X, end.Y, end.Z, color.R, color.G, color.B });
    }
}

namespace DevTools::Viewer::Map::System
{
    auto RaycastPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const ViewTable* views, const std::vector<std::size_t>& rows, std::uint64_t generation, const FramePoses& poses) -> void
    {
        if (!device || !context) return;

        const std::uint32_t key{ ShownRowsKeyOf(rows, poses.Revision) };
        if (m_Gate.IsCurrent(generation, key)) return;

        m_Gate.Mark(generation, key);
        m_VertexCount = 0;

        if (!views) return;

        std::vector<Vertex> vertices{};

        for (const std::size_t row : rows)
        {
            if (row >= views->size() || !(*views)[row].Head.IsComputed) continue;

            const View& view{ (*views)[row] };
            const Raycasts raycasts{ RaycastsOf(view) };
            const Vec3 originOffset{ poses.OffsetOf(view.Head.BipedHandle) };

            AppendRay(vertices, raycasts.AimHit, true, originOffset);

            for (const auto& ray : raycasts.PerceptionHits)
            {
                AppendRay(vertices, ray, false, originOffset);
            }
        }

        if (vertices.empty()) return;

        const UINT needed = static_cast<UINT>(vertices.size());

        if (!Engine::Render::System::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, m_VertexBuffer, m_Capacity, "[RaycastPass]", m_Log))
        {
            return;
        }

        if (!Engine::Render::System::UploadDynamicVertices(context, m_VertexBuffer.Get(),
            vertices, "[RaycastPass]", m_Log))
        {
            return;
        }

        m_VertexCount = needed;
    }

    auto RaycastPass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

        pipeline.Bind(context, VertexLayout::Colored);

        Engine::Render::System::DrawVertexBuffer(context, m_VertexBuffer.Get(),
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