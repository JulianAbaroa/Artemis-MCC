module;

#include <d3d11.h>

module DevTools.Viewer.Map.System;
import :ObsPass;

import Common.Math.Type;
import Common.Team.Type;
import Engine.Render.Type;
import Engine.Render.System;
import Perception.Details.Type;
import Views.View.Type;
import Views.View.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Geometry.System;
import DevTools.Viewer.Interpolation.Type;
import DevTools.Viewer.Map.Type;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using Engine::Render::Type::Vertex;
    using Engine::Render::Type::VertexLayout;
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;
    using Views::View::Type::ObsTargets;
    using Views::View::Type::View;
    using Views::View::System::ObsTargetsOf;
    using Views::View::System::RivalTeamOf;
    using DevTools::Viewer::Geometry::System::AppendArrow;
    using DevTools::Viewer::Options::Type::Level;
    using DevTools::Viewer::Style::Type::ColorOfTeam;

    using DevTools::Viewer::Map::Type::k_InitialCapacity;
    using DevTools::Viewer::Map::Type::k_MarkerLift;

    auto Raised(const Vec3& p) -> Vec3
    {
        return Vec3{ p.X, p.Y, p.Z + k_MarkerLift };
    }
}

namespace DevTools::Viewer::Map::System
{
    auto ObsPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context, const Tick* tick, const std::vector<std::size_t>& agentRows,
        Level level, const FramePoses& poses) -> void
    {
        if (!device || !context) return;

        // note: The vertices are the same at Minimal and Full (the distances of Full are labels), but the level is in the key all the same: a layer that
        // is switched off and on again is uploaded again.
        const std::uint32_t key{ poses.Revision * 4u + static_cast<std::uint32_t>(level) };
        const std::uint64_t generation{ tick ? tick->Generation : 0 };
        if (m_Gate.IsCurrent(generation, key)) return;

        m_Gate.Mark(generation, key);
        m_VertexCount = 0;

        if (!tick || !tick->Views || level == Level::Off) return;

        std::vector<Vertex> vertices{};

        for (const std::size_t row : agentRows)
        {
            if (row >= tick->Views->size()) continue;

            const View& view{ (*tick->Views)[row] };
            const ObsTargets targets{ ObsTargetsOf(view) };
            if (!targets.HasFlag && !targets.HasStand) continue;

            const Vec3 offset{ poses.OffsetOf(view.Head.BipedHandle) };
            const Vec3 from{ Raised(Vec3{ targets.From.X + offset.X, targets.From.Y + offset.Y, targets.From.Z + offset.Z }) };

            if (targets.HasFlag)
            {
                const Team rival{ tick->ObjectiveDetails ? RivalTeamOf(*tick->ObjectiveDetails, view.Head.Team) : Team::Neutral };
                AppendArrow(vertices, from, Raised(targets.Flag), ColorOfTeam(rival), false);
            }

            if (targets.HasStand) AppendArrow(vertices, from, Raised(targets.Stand), ColorOfTeam(view.Head.Team), false);
        }

        if (vertices.empty()) return;

        const UINT needed{ static_cast<UINT>(vertices.size()) };

        if (!Engine::Render::System::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, m_VertexBuffer, m_Capacity, "[ObsPass]", m_Log))
        {
            return;
        }

        if (!Engine::Render::System::UploadDynamicVertices(context, m_VertexBuffer.Get(),
            vertices, "[ObsPass]", m_Log))
        {
            return;
        }

        m_VertexCount = needed;
    }

    auto ObsPass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

        pipeline.Bind(context, VertexLayout::Colored);

        Engine::Render::System::DrawVertexBuffer(context, m_VertexBuffer.Get(),
            m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    auto ObsPass::Release() -> void
    {
        m_VertexBuffer.Reset();
        m_Capacity = 0;
        m_VertexCount = 0;

        m_Gate.Reset();
    }
}