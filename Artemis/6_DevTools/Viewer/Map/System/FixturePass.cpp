module;

#include <d3d11.h>

module DevTools.Viewer.Map.System;
import :FixturePass;

import Common.Math.Type;
import Common.Math.System;
import Engine.Render.Type;
import Engine.Render.System;
import Perception.Details.Type;
import Perception.Entities.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Geometry.System;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using Engine::Render::Type::Vertex;
    using Common::Math::Type::Vec3;
    using Common::Math::System::Add;
    using Common::Math::System::Length;
    using Common::Math::System::Scale;
    using DevTools::Viewer::Geometry::System::AppendArrow;
    using DevTools::Viewer::Style::Type::Color;

    using Engine::Render::Type::VertexLayout;

    using DevTools::Viewer::Style::Type::k_Fixture;

    constexpr UINT k_InitialCapacity{ 1024 };
    constexpr std::size_t k_MaxDestinations{ 8 };
    constexpr float k_AnchorLift{ 0.4f };

    // return: The point lifted so the arrows do not sink into the floor.
    auto Raised(const Vec3& p) -> Vec3
    {
        return { p.X, p.Y, p.Z + k_AnchorLift };
    }

    auto AppendDirectional(std::vector<Vertex>& vertices, const Vec3& origin, const Vec3& direction,
        float length, const Color& c) -> void
    {
        const float magnitude = Length(direction);
        if (magnitude <= 0.0f) return;

        const Vec3 from = Raised(origin);
        AppendArrow(vertices, from, Add(from, Scale(direction, length / magnitude)), c, false);
    }
}

namespace DevTools::Viewer::Map::System
{
    auto FixturePass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const std::shared_ptr<const Tick>& tick, const FixturePassOptions& options) -> void
    {
        m_VertexCount = 0;

        if (!device || !context || !tick) return;

        std::vector<Vertex> vertices{};

        if (options.TeleportLinks && tick->TeleporterDetails && tick->Entities)
        {
            for (const auto& [handle, teleporter] : *tick->TeleporterDetails)
            {
                const auto* from{ tick->Entities->Find(handle) };
                if (!from) continue;

                const std::size_t count{ (std::min)(teleporter.Destinations.size(), k_MaxDestinations) };

                for (std::size_t i = 0; i < count; ++i)
                {
                    const auto* to{ tick->Entities->Find(teleporter.Destinations[i]) };
                    if (!to) continue;

                    AppendArrow(vertices, Raised(from->WorldOrigin), Raised(to->WorldOrigin), k_Fixture, true);
                }
            }
        }

        if (options.LiftArrows && tick->LiftDetails && tick->Entities)
        {
            for (const auto& [handle, lift] : *tick->LiftDetails)
            {
                const auto* entity{ tick->Entities->Find(handle) };
                if (!entity) continue;

                if (lift.Stages.empty()) continue;

                AppendDirectional(vertices, entity->BoundingCenter, lift.Stages.back().Direction, options.ArrowLength, k_Fixture);
            }
        }

        // note: Only OneWay barriers have a pass direction. A zero direction draws nothing.
        if (options.ShieldArrows && tick->BarrierDetails && tick->Entities)
        {
            for (const auto& [handle, barrier] : *tick->BarrierDetails)
            {
                const auto* entity{ tick->Entities->Find(handle) };
                if (!entity) continue;

                AppendDirectional(vertices, entity->BoundingCenter, barrier.PassDirection, options.ArrowLength, k_Fixture);
            }
        }

        if (vertices.empty()) return;

        const UINT needed = static_cast<UINT>(vertices.size());

        if (!Engine::Render::System::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, m_VertexBuffer, m_Capacity, "[FixturePass]", m_Log))
        {
            return;
        }

        if (!Engine::Render::System::UploadDynamicVertices(context, m_VertexBuffer.Get(),
            vertices, "[FixturePass]", m_Log))
        {
            return;
        }

        m_VertexCount = needed;
    }

    auto FixturePass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

        pipeline.BindLines(context, VertexLayout::Colored);

        Engine::Render::System::DrawVertexBuffer(context, m_VertexBuffer.Get(),
            m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    auto FixturePass::Release() -> void
    {
        m_VertexBuffer.Reset();
        m_Capacity = 0;
        m_VertexCount = 0;
    }
}