module;

#include <d3d11.h>
#include <wrl/client.h>

module Viewer.Map.System;
import :ZonePass;

import Platform.Render.Type;
import Platform.Render.System;
import Common.Math.Type;
import Common.ZoneShape.Type;
import Viewer.Geometry.System;
import Viewer.Style.Type;
import std;

namespace
{
    using Vertex = Platform::Render::Type::Vertex;
    using Vec3 = Common::Math::Type::Vec3;
    using ZoneShape = Common::ZoneShape::Type::ZoneShape;
    using ZoneKind = Common::ZoneShape::Type::Kind;

    using Platform::Render::Type::SurfaceMode;
    using Platform::Render::Type::VertexLayout;

    using Viewer::Style::Type::k_TranslucentAlpha;

    constexpr UINT k_InitialCapacity{ 2048 };
}

namespace Viewer::Map::System
{
    auto ZonePass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const std::shared_ptr<const Tick>& tick, const PaletteService& palette) -> void
    {
        if (!device || !context) return;

        const std::uint64_t generation = tick ? tick->Generation : 0;
        if (tick && m_Gate.IsCurrent(generation)) return;

        m_Gate.Mark(generation);
        m_VertexCount = 0;

        if (!tick || !tick->Fixtures) return;

        const auto& fixtures = *tick->Fixtures;

        std::vector<Vertex>& vertices = m_Scratch;
        vertices.clear();

        auto emit = [&](std::uint32_t handle, const Vec3& position,
            const Vec3& forward, const Vec3& up, const ZoneShape& zone)
        {
            if (zone.Kind == ZoneKind::None) return;

            Viewer::Geometry::System::ZoneGeometry::AppendSolid(vertices, position, forward, up,
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

        if (!Platform::Render::System::GpuBuffer::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, m_VertexBuffer, m_Capacity, "[ZonePass]", m_LogsService))
        {
            return;
        }

        if (!Platform::Render::System::GpuBuffer::UploadDynamicVertices(context, m_VertexBuffer.Get(),
            vertices, "[ZonePass]", m_LogsService))
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

        Platform::Render::System::GpuBuffer::DrawVertexBuffer(context, m_VertexBuffer.Get(),
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