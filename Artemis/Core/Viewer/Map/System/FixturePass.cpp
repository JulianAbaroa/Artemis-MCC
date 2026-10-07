module;

#include <d3d11.h>

module Viewer.Map.System;
import :FixturePass;

import Platform.Render.Type;
import Platform.Render.System;
import Common.Math.Type;
import Environment.Fixtures.Type;
import Viewer.Style.Type;
import std;

namespace
{
    using Vertex = Platform::Render::Type::Vertex;
    using Vec3 = Common::Math::Type::Vec3;
    using Color = Viewer::Style::Type::Color;

    using Platform::Render::Type::VertexLayout;

    using Viewer::Style::Type::k_Teleporter;
    using Viewer::Style::Type::k_Lift;
    using Viewer::Style::Type::k_Shield;

    constexpr UINT k_InitialCapacity{ 1024 };
    constexpr std::size_t k_MaxDestinations{ 8 };
    constexpr float k_DashLength{ 0.6f };
    constexpr float k_DashGap{ 0.4f };
    constexpr float k_HeadLength{ 0.45f };
    constexpr float k_HeadWidth{ 0.2f };
    constexpr float k_AnchorLift{ 0.4f };
    constexpr float k_Minimum{ 1e-5f };

    auto Sub(const Vec3& a, const Vec3& b) -> Vec3
    {
        return { a.X - b.X, a.Y - b.Y, a.Z - b.Z };
    }
    auto Add(const Vec3& a, const Vec3& b) -> Vec3
    {
        return { a.X + b.X, a.Y + b.Y, a.Z + b.Z };
    }
    auto Scale(const Vec3& a, float s) -> Vec3
    {
        return { a.X * s, a.Y * s, a.Z * s };
    }
    auto Length(const Vec3& a) -> float
    {
        return std::sqrt(a.X * a.X + a.Y * a.Y + a.Z * a.Z);
    }

    auto Cross(const Vec3& a, const Vec3& b) -> Vec3
    {
        return { a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X };
    }

    // return: The point lifted so the arrows do not sink into the floor.
    auto Raised(const Vec3& p) -> Vec3
    {
        return { p.X, p.Y, p.Z + k_AnchorLift };
    }

    auto AppendLine(std::vector<Vertex>& vertices, const Vec3& a, const Vec3& b, const Color& c) -> void
    {
        vertices.push_back(Vertex{ a.X, a.Y, a.Z, c.R, c.G, c.B });
        vertices.push_back(Vertex{ b.X, b.Y, b.Z, c.R, c.G, c.B });
    }

    auto AppendHead(std::vector<Vertex>& vertices, const Vec3& tip, const Vec3& dir, const Color& c) -> void
    {
        Vec3 side = Cross(dir, Vec3{ 0.0f, 0.0f, 1.0f });
        if (Length(side) < 0.1f) side = Cross(dir, Vec3{ 1.0f, 0.0f, 0.0f });

        side = Scale(side, 1.0f / Length(side));
        const Vec3 other = Cross(dir, side);
        const Vec3 baseCenter = Sub(tip, Scale(dir, k_HeadLength));

        AppendLine(vertices, tip, Add(baseCenter, Scale(side, k_HeadWidth)), c);
        AppendLine(vertices, tip, Sub(baseCenter, Scale(side, k_HeadWidth)), c);
        AppendLine(vertices, tip, Add(baseCenter, Scale(other, k_HeadWidth)), c);
        AppendLine(vertices, tip, Sub(baseCenter, Scale(other, k_HeadWidth)), c);
    }

    // Appends an arrow with a head at the tip. A dashed arrow is drawn as separate segments.
    auto AppendArrow(std::vector<Vertex>& vertices, const Vec3& from, const Vec3& to,
        const Color& c, bool dashed) -> void
    {
        const Vec3 delta = Sub(to, from);
        const float length = Length(delta);
        if (length < k_Minimum) return;

        const Vec3 dir = Scale(delta, 1.0f / length);

        if (!dashed)
        {
            AppendLine(vertices, from, to, c);
        }
        else
        {
            for (float at = 0.0f; at < length; at += k_DashLength + k_DashGap)
            {
                const float end = (std::min)(at + k_DashLength, length);
                AppendLine(vertices, Add(from, Scale(dir, at)), Add(from, Scale(dir, end)), c);
            }
        }

        AppendHead(vertices, to, dir, c);
    }

    auto AppendDirectional(std::vector<Vertex>& vertices, const Vec3& origin, const Vec3& direction,
        float length, const Color& c) -> void
    {
        const float magnitude = Length(direction);
        if (magnitude < k_Minimum) return;

        const Vec3 from = Raised(origin);
        AppendArrow(vertices, from, Add(from, Scale(direction, length / magnitude)), c, false);
    }
}

namespace Viewer::Map::System
{
    auto FixturePass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const std::shared_ptr<const Fixtures>& fixtures, const FixturePassOptions& options) -> void
    {
        m_VertexCount = 0;

        if (!device || !context || !fixtures) return;

        std::vector<Vertex> vertices{};

        if (options.TeleportLinks)
        {
            for (const auto& teleport : fixtures->Teleporters)
            {
                const std::size_t count = (std::min)(teleport.DestinationPositions.size(), k_MaxDestinations);

                for (std::size_t i = 0; i < count; ++i)
                {
                    AppendArrow(vertices, Raised(teleport.Position), Raised(teleport.DestinationPositions[i]),
                        k_Teleporter, true);
                }
            }
        }

        if (options.LiftArrows)
        {
            for (const auto& lift : fixtures->Lifts)
            {
                AppendDirectional(vertices, lift.Position, lift.LaunchDirection,
                    options.ArrowLength, k_Lift);
            }
        }

        if (options.ShieldArrows)
        {
            for (const auto& shield : fixtures->Shields)
            {
                if (!shield.BlockDirection) continue;

                AppendDirectional(vertices, shield.Position, *shield.BlockDirection,
                    options.ArrowLength, k_Shield);
            }
        }

        if (vertices.empty()) return;

        const UINT needed = static_cast<UINT>(vertices.size());

        if (!Platform::Render::System::GpuBuffer::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, m_VertexBuffer, m_Capacity, "[FixturePass]", m_LogsService))
        {
            return;
        }

        if (!Platform::Render::System::GpuBuffer::UploadDynamicVertices(context, m_VertexBuffer.Get(),
            vertices, "[FixturePass]", m_LogsService))
        {
            return;
        }

        m_VertexCount = needed;
    }

    auto FixturePass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

        pipeline.Bind(context, VertexLayout::Colored);

        Platform::Render::System::GpuBuffer::DrawVertexBuffer(context, m_VertexBuffer.Get(),
            m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    auto FixturePass::Release() -> void
    {
        m_VertexBuffer.Reset();
        m_Capacity = 0;
        m_VertexCount = 0;
    }
}