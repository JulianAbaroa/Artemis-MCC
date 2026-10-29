module;

#include <d3d11.h>

module DevTools.Viewer.Map.System;
import :ZonePass;

import Common.Math.Type;
import Common.ZoneShape.Type;
import Engine.Render.Type;
import Engine.Render.System;
import DevTools.Viewer.Geometry.System;
import DevTools.Viewer.Map.Type;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using Engine::Render::Type::Vertex;
    using Common::Math::Type::Vec3;
    using Common::ZoneShape::Type::ZoneShape;
    using Common::ZoneShape::Type::ZoneShapeKind;

    using Engine::Render::Type::SurfaceMode;
    using Engine::Render::Type::VertexLayout;

    using DevTools::Viewer::Style::Type::k_TranslucentAlpha;
    using DevTools::Viewer::Map::Type::k_InitialCapacity;
}

namespace DevTools::Viewer::Map::System
{
    auto ZonePass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const std::shared_ptr<const Tick>& tick, const PaletteService& palette) -> void
    {
        if (!device || !context) return;

        const std::uint64_t generation = tick ? tick->Generation : 0;
        if (tick && m_Gate.IsCurrent(generation)) return;

        m_Gate.Mark(generation);
        m_VertexCount = 0;

        if (!tick || !tick->ZoneDetails || !tick->Entities) return;

        std::vector<Vertex>& vertices = m_Scratch;
        vertices.clear();

        auto emit = [&](std::uint32_t handle, const Vec3& position,
            const Vec3& forward, const Vec3& up, const ZoneShape& zone)
        {
            if (zone.Kind == ZoneShapeKind::None) return;

            DevTools::Viewer::Geometry::System::AppendSolid(vertices, position, forward, up,
                zone, palette.ColorOf(handle));
        };

        // note: Every zone, teleporters included, comes from the ZoneDetails. The pose is the WorldOrigin of its entity (D88).
        for (const auto& [handle, zone] : *tick->ZoneDetails)
        {
            const auto* entity = tick->Entities->Find(handle);
            if (!entity) continue;

            emit(handle, entity->WorldOrigin, entity->Forward, entity->Up, zone.Shape);
        }

        if (vertices.empty()) return;

        const UINT needed = static_cast<UINT>(vertices.size());

        if (!Engine::Render::System::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, m_VertexBuffer, m_Capacity, "[ZonePass]", m_Log))
        {
            return;
        }

        if (!Engine::Render::System::UploadDynamicVertices(context, m_VertexBuffer.Get(),
            vertices, "[ZonePass]", m_Log))
        {
            return;
        }

        m_VertexCount = needed;
    }

    auto ZonePass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

        pipeline.Bind(context, VertexLayout::Colored, SurfaceMode::Translucent,
            k_TranslucentAlpha);

        Engine::Render::System::DrawVertexBuffer(context, m_VertexBuffer.Get(),
            m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    auto ZonePass::Release() -> void
    {
        m_VertexBuffer.Reset();
        m_Capacity = 0;
        m_VertexCount = 0;

        m_Gate.Reset();

        std::vector<Vertex>().swap(m_Scratch);
    }
}