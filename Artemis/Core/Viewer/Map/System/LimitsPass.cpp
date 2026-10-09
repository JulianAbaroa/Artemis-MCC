module;

#include <d3d11.h>

module Viewer.Map.System;
import :LimitsPass;

import Platform.Render.Type;
import Platform.Render.System;
import Common.Math.Type;
import Resolved.Definitions.Type;
import Viewer.Style.Type;
import std;

namespace
{
    using Vertex = Platform::Render::Type::Vertex;
    using Vec3 = Common::Math::Type::Vec3;
    using Color = Viewer::Style::Type::Color;
    using TriggerVolume = Resolved::Definitions::Type::Scnr::TriggerVolume;
    using TriggerVolumeKind = Resolved::Definitions::Type::Scnr::TriggerVolumeKind;
    using BoundaryTrigger = Resolved::Definitions::Type::Scnr::BoundaryTrigger;
    using Scnr = Resolved::Definitions::Type::Scnr::Scnr;
    using SoftCeilingKind = Resolved::Definitions::Type::Scnr::SoftCeilingKind;
    using SoftCeilingMesh = Resolved::Definitions::Type::Sddt::SoftCeilingMesh;

    using Platform::Render::Type::SurfaceMode;
    using Platform::Render::Type::VertexLayout;

    using Viewer::Style::Type::k_LimitKill;
    using Viewer::Style::Type::k_LimitSoftKill;
    using Viewer::Style::Type::k_LimitSafe;
    using Viewer::Style::Type::k_CeilingAcceleration;
    using Viewer::Style::Type::k_CeilingSoftKill;
    using Viewer::Style::Type::k_CeilingSlipSurface;

    constexpr UINT k_InitialCapacity{ 2048 };
    constexpr float k_Minimum{ 1e-4f };

    // The volumes are static, so the generation of the gate is always the same.
    constexpr std::uint64_t k_Generation{ 1 };

    // Triangles and edges of the volumes being built.
    struct Mesh
    {
        std::vector<Vertex>& Faces;
        std::vector<Vertex>& Edges;
    };

    auto Add(const Vec3& a, const Vec3& b) -> Vec3
    {
        return { a.X + b.X, a.Y + b.Y, a.Z + b.Z };
    }
    auto Scale(const Vec3& a, float s) -> Vec3
    {
        return { a.X * s, a.Y * s, a.Z * s };
    }
    auto Cross(const Vec3& a, const Vec3& b) -> Vec3
    {
        return { a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X };
    }

    auto MakeVertex(const Vec3& p, const Color& c) -> Vertex
    {
        return Vertex{ p.X, p.Y, p.Z, c.R, c.G, c.B };
    }

    auto AppendLine(Mesh& mesh, const Vec3& a, const Vec3& b, const Color& c) -> void
    {
        mesh.Edges.push_back(MakeVertex(a, c));
        mesh.Edges.push_back(MakeVertex(b, c));
    }

    auto AppendTriangle(Mesh& mesh, const Vec3& a, const Vec3& b, const Vec3& c, const Color& color) -> void
    {
        mesh.Faces.push_back(MakeVertex(a, color));
        mesh.Faces.push_back(MakeVertex(b, color));
        mesh.Faces.push_back(MakeVertex(c, color));
    }

    auto AppendQuad(Mesh& mesh, const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Color& color) -> void
    {
        AppendTriangle(mesh, a, b, c, color);
        AppendTriangle(mesh, a, c, d, color);
    }

    // Appends the six faces and the twelve edges of a box given by its eight corners.
    // param corners: Bit 0 steps along the first axis, bit 1 along the second and bit 2 along the third.
    auto AppendBox(Mesh& mesh, const std::array<Vec3, 8>& corners, const Color& color) -> void
    {
        for (std::size_t corner{ 0 }; corner < corners.size(); ++corner)
        {
            for (std::size_t axis{ 1 }; axis <= 4; axis <<= 1)
            {
                if ((corner & axis) != 0) continue;

                AppendLine(mesh, corners[corner], corners[corner | axis], color);
            }
        }

        for (std::size_t fixed{ 1 }; fixed <= 4; fixed <<= 1)
        {
            const std::size_t first{ fixed == 1 ? std::size_t{ 2 } : std::size_t{ 1 } };
            const std::size_t second{ fixed == 4 ? std::size_t{ 2 } : std::size_t{ 4 } };

            for (std::size_t side{ 0 }; side < 2; ++side)
            {
                const std::size_t origin{ side == 0 ? std::size_t{ 0 } : fixed };

                AppendQuad(mesh, corners[origin], corners[origin | first],
                    corners[origin | first | second], corners[origin | second], color);
            }
        }
    }

    // Splits a polygon into triangles by cutting its ears. The polygon is read on the XY plane.
    // return: Triples of indices into the points. A polygon that cannot be cut ends as a fan.
    auto Triangulate(const std::vector<Vec3>& points) -> std::vector<std::array<std::size_t, 3>>
    {
        std::vector<std::array<std::size_t, 3>> triangles{};
        if (points.size() < 3) return triangles;

        const auto cross = [&](std::size_t a, std::size_t b, std::size_t c)
        {
            return (points[b].X - points[a].X) * (points[c].Y - points[a].Y)
                - (points[b].Y - points[a].Y) * (points[c].X - points[a].X);
        };

        float area{ 0.0f };
        for (std::size_t index{ 0 }; index < points.size(); ++index)
        {
            const Vec3& from{ points[index] };
            const Vec3& to{ points[(index + 1) % points.size()] };
            area += from.X * to.Y - to.X * from.Y;
        }
        const float winding{ area < 0.0f ? -1.0f : 1.0f };

        std::vector<std::size_t> remaining(points.size());
        std::iota(remaining.begin(), remaining.end(), std::size_t{ 0 });

        while (remaining.size() > 3)
        {
            bool isCut{ false };

            for (std::size_t at{ 0 }; at < remaining.size(); ++at)
            {
                const std::size_t previous{ remaining[(at + remaining.size() - 1) % remaining.size()] };
                const std::size_t current{ remaining[at] };
                const std::size_t next{ remaining[(at + 1) % remaining.size()] };

                if (cross(previous, current, next) * winding <= k_Minimum) continue;

                bool isEar{ true };

                for (const std::size_t other : remaining)
                {
                    if (other == previous || other == current || other == next) continue;

                    if (cross(previous, current, other) * winding >= 0.0f
                        && cross(current, next, other) * winding >= 0.0f
                        && cross(next, previous, other) * winding >= 0.0f)
                    {
                        isEar = false;
                        break;
                    }
                }

                if (!isEar) continue;

                triangles.push_back({ previous, current, next });
                remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(at));
                isCut = true;
                break;
            }

            if (!isCut) break;
        }

        for (std::size_t index{ 1 }; index + 1 < remaining.size(); ++index)
        {
            triangles.push_back({ remaining[0], remaining[index], remaining[index + 1] });
        }

        return triangles;
    }

    // A bounding box volume starts at its position, which is a corner, and spans its extents along forward, left and up.
    auto AppendBoundingBox(Mesh& mesh, const TriggerVolume& volume, const Color& color) -> void
    {
        const Vec3 left{ Cross(volume.Up, volume.Forward) };

        const Vec3 alongForward{ Scale(volume.Forward, volume.Extents.X) };
        const Vec3 alongLeft{ Scale(left, volume.Extents.Y) };
        const Vec3 alongUp{ Scale(volume.Up, volume.Extents.Z) };

        const Vec3& origin{ volume.Position };

        std::array<Vec3, 8> corners{};

        for (std::size_t corner{ 0 }; corner < corners.size(); ++corner)
        {
            Vec3 point{ origin };

            if ((corner & 1) != 0) point = Add(point, alongForward);
            if ((corner & 2) != 0) point = Add(point, alongLeft);
            if ((corner & 4) != 0) point = Add(point, alongUp);

            corners[corner] = point;
        }

        AppendBox(mesh, corners, color);
    }

    // A sector volume is its points extruded between the bottom and the top of its bounds.
    // note: Without a height, only the polygon at the height of its points is drawn.
    auto AppendSector(Mesh& mesh, const TriggerVolume& volume, const Color& color) -> void
    {
        const std::vector<Vec3>& points{ volume.SectorPoints };
        if (points.size() < 2) return;

        const float bottom{ volume.SectorBoundsMin.Z };
        const float top{ volume.SectorBoundsMax.Z };
        const bool hasHeight{ top - bottom > k_Minimum };

        const auto at = [&](const Vec3& point, float height)
        {
            return Vec3{ point.X, point.Y, hasHeight ? height : point.Z };
        };

        for (std::size_t index{ 0 }; index < points.size(); ++index)
        {
            const Vec3& from{ points[index] };
            const Vec3& to{ points[(index + 1) % points.size()] };

            AppendLine(mesh, at(from, bottom), at(to, bottom), color);

            if (!hasHeight) continue;

            AppendLine(mesh, at(from, top), at(to, top), color);
            AppendLine(mesh, at(from, bottom), at(from, top), color);

            AppendQuad(mesh, at(from, bottom), at(to, bottom), at(to, top), at(from, top), color);
        }

        for (const auto& [first, second, third] : Triangulate(points))
        {
            AppendTriangle(mesh, at(points[first], bottom), at(points[second], bottom), at(points[third], bottom), color);

            if (hasHeight)
            {
                AppendTriangle(mesh, at(points[first], top), at(points[second], top), at(points[third], top), color);
            }
        }
    }

    auto AppendVolume(Mesh& mesh, const TriggerVolume& volume, const Color& color) -> void
    {
        switch (volume.Kind)
        {
        case TriggerVolumeKind::BoundingBox:
            AppendBoundingBox(mesh, volume, color);
            break;

        case TriggerVolumeKind::Sector:
            AppendSector(mesh, volume, color);
            break;

        default:
            break;
        }
    }

    // Appends the volumes that the triggers point to, once each.
    // param immediateColor: Color of a volume that acts at once.
    // param delayedColor: Color of a volume that acts after a countdown. A volume that any trigger makes immediate is immediate.
    auto AppendTriggers(Mesh& mesh, const Scnr& scnr, const std::vector<BoundaryTrigger>& triggers,
        const Color& immediateColor, const Color& delayedColor) -> void
    {
        std::unordered_map<std::int16_t, bool> isDelayed{};

        for (const BoundaryTrigger& trigger : triggers)
        {
            const std::int16_t index{ trigger.TriggerVolumeIndex };

            if (index < 0 || static_cast<std::size_t>(index) >= scnr.TriggerVolumes.size()) continue;

            const auto [at, isNew] = isDelayed.try_emplace(index, trigger.DontKillImmediately);

            if (!isNew && !trigger.DontKillImmediately)
            {
                at->second = false;
            }
        }

        for (const auto& [index, delayed] : isDelayed)
        {
            AppendVolume(mesh, scnr.TriggerVolumes[static_cast<std::size_t>(index)],
                delayed ? delayedColor : immediateColor);
        }
    }

    auto ColorOfCeiling(SoftCeilingKind kind) -> Color
    {
        switch (kind)
        {
        case SoftCeilingKind::SoftKill: return k_CeilingSoftKill;
        case SoftCeilingKind::SlipSurface: return k_CeilingSlipSurface;
        default: return k_CeilingAcceleration;
        }
    }

    // A soft ceiling has no outline, because its triangles would only fill the view with lines.
    auto AppendCeiling(Mesh& mesh, const SoftCeilingMesh& ceiling) -> void
    {
        const Color color{ ColorOfCeiling(ceiling.Kind) };

        for (const auto& triangle : ceiling.Triangles)
        {
            AppendTriangle(mesh, triangle.A, triangle.B, triangle.C, color);
        }
    }

    auto KeyOf(const Viewer::Map::Type::LimitsPassOptions& options, bool hasScnrs, bool hasSddts) -> std::uint32_t
    {
        return (options.KillVolumes ? 1u : 0u)
            | (options.SafeVolumes ? 2u : 0u)
            | (options.SoftCeilings ? 4u : 0u)
            | (hasScnrs ? 8u : 0u)
            | (hasSddts ? 16u : 0u);
    }
}

namespace Viewer::Map::System
{
    auto LimitsPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const std::shared_ptr<const MapScnrs>& scnrs, const std::shared_ptr<const MapSddts>& sddts,
        const LimitsPassOptions& options) -> void
    {
        if (!device || !context) return;

        m_Opacity = options.Opacity;

        const std::uint32_t key{ KeyOf(options, scnrs != nullptr, sddts != nullptr) };
        if (m_Gate.IsCurrent(k_Generation, key)) return;

        m_Gate.Mark(k_Generation, key);
        m_Faces.VertexCount = 0;
        m_Edges.VertexCount = 0;

        m_Faces.Scratch.clear();
        m_Edges.Scratch.clear();

        Mesh mesh{ m_Faces.Scratch, m_Edges.Scratch };

        if (scnrs)
        {
            for (const auto& [tagName, scnr] : *scnrs)
            {
                if (options.KillVolumes)
                {
                    AppendTriggers(mesh, scnr, scnr.KillTriggers, k_LimitKill, k_LimitSoftKill);
                }

                if (options.SafeVolumes)
                {
                    AppendTriggers(mesh, scnr, scnr.SafeZoneTriggers, k_LimitSafe, k_LimitSafe);
                }
            }
        }

        if (sddts && options.SoftCeilings)
        {
            for (const auto& [tagName, sddt] : *sddts)
            {
                for (const SoftCeilingMesh& ceiling : sddt.SoftCeilings)
                {
                    AppendCeiling(mesh, ceiling);
                }
            }
        }

        this->UploadBatch(device, context, m_Faces, "[LimitsPass] faces");
        this->UploadBatch(device, context, m_Edges, "[LimitsPass] edges");
    }

    auto LimitsPass::UploadBatch(ID3D11Device* device, ID3D11DeviceContext* context, Batch& batch,
        const char* tag) -> bool
    {
        if (batch.Scratch.empty()) return true;

        const UINT needed = static_cast<UINT>(batch.Scratch.size());

        if (!Platform::Render::System::GpuBuffer::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, batch.Buffer, batch.Capacity, tag, m_LogsService))
        {
            return false;
        }

        if (!Platform::Render::System::GpuBuffer::UploadDynamicVertices(context, batch.Buffer.Get(),
            batch.Scratch, tag, m_LogsService))
        {
            return false;
        }

        batch.VertexCount = needed;

        return true;
    }

    auto LimitsPass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context) return;

        if (m_Faces.Buffer && m_Faces.VertexCount != 0)
        {
            pipeline.Bind(context, VertexLayout::Colored, SurfaceMode::Translucent, m_Opacity);

            Platform::Render::System::GpuBuffer::DrawVertexBuffer(context, m_Faces.Buffer.Get(),
                m_Faces.VertexCount, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }

        if (m_Edges.Buffer && m_Edges.VertexCount != 0)
        {
            pipeline.Bind(context, VertexLayout::Colored);

            Platform::Render::System::GpuBuffer::DrawVertexBuffer(context, m_Edges.Buffer.Get(),
                m_Edges.VertexCount, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        }
    }

    auto LimitsPass::Release() -> void
    {
        for (Batch* batch : { &m_Faces, &m_Edges })
        {
            batch->Buffer.Reset();
            batch->Capacity = 0;
            batch->VertexCount = 0;
            std::vector<Vertex>().swap(batch->Scratch);
        }

        m_Gate.Reset();
    }
}