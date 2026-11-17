module;

#include <d3d11.h>

module DevTools.Viewer.Map.System;
import :ViewPass;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Engine.Render.Type;
import Engine.Render.System;
import Perception.Entities.Type;
import Perception.Players.Type;
import Views.Raycast.Type;
import Views.View.Type;
import Views.View.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Geometry.System;
import DevTools.Viewer.Map.Type;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using Engine::Render::Type::Vertex;
    using Engine::Render::Type::VertexLayout;
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::FlagState;
    using Bridge::Codec::Tick::Type::Tick;
    using Perception::Players::Type::Player;
    using Views::View::Type::View;
    using Views::View::Type::ViewChannel;
    using Views::View::Type::ViewFlag;
    using Views::View::Type::ViewMode;
    using Views::View::Type::ViewOther;
    using Views::View::Type::ViewRelation;
    using Views::View::System::AimDirectionOf;
    using Views::View::System::ViewPositionOf;
    using DevTools::Viewer::Geometry::System::AppendArrow;
    using DevTools::Viewer::Geometry::System::AppendCircle;
    using DevTools::Viewer::Geometry::System::AppendCone;
    using DevTools::Viewer::Geometry::System::AppendCross;
    using DevTools::Viewer::Geometry::System::AppendLine;
    using DevTools::Viewer::Style::Type::Color;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
    using Views::Raycast::Type::Sight::k_SightHalfAngle;
    using Views::Raycast::Type::Sight::k_SightRange;
    using DevTools::Viewer::Map::Type::k_InitialCapacity;
    using DevTools::Viewer::Style::Type::k_Selected;
    using DevTools::Viewer::Style::Type::k_ViewAlly;
    using DevTools::Viewer::Style::Type::k_ViewAllyDim;
    using DevTools::Viewer::Style::Type::k_ViewCone;
    using DevTools::Viewer::Style::Type::k_ViewEnemy;
    using DevTools::Viewer::Style::Type::k_ViewHidden;
    using DevTools::Viewer::Style::Type::k_ViewNone;
    using DevTools::Viewer::Style::Type::k_ViewRadar;
    using DevTools::Viewer::Style::Type::k_ViewSound;

    constexpr int k_ConeLines{ 6 };

    // World units. A biped is about 0.7 tall, so the marker is a small cross at its middle and a ring at its feet.
    constexpr float k_MarkerHalfSize{ 0.25f };
    constexpr float k_MarkerLift{ 0.35f };
    constexpr float k_RingRadius{ 0.35f };
    constexpr int k_RingSegments{ 16 };
    constexpr float k_FlagHalfSize{ 0.5f };
    constexpr float k_CarrierRingRadius{ 0.7f };
    constexpr float k_ElevationArrow{ 1.5f };

    // Sounds: the arrow is k_SoundMinLength long and grows up to k_SoundMaxLength with the intensity (clamped to 0..1).
    constexpr float k_SoundMinLength{ 2.0f };
    constexpr float k_SoundMaxLength{ 10.0f };

    constexpr Vec3 k_Up{ 0.0f, 0.0f, 1.0f };

    auto IsZero(const Vec3& v) -> bool
    {
        return v.X == 0.0f && v.Y == 0.0f && v.Z == 0.0f;
    }

    auto Raised(const Vec3& p, float lift) -> Vec3
    {
        return Vec3{ p.X, p.Y, p.Z + lift };
    }

    auto ColorOfRelation(ViewRelation relation) -> Color
    {
        switch (relation)
        {
        case ViewRelation::Enemy: return k_ViewEnemy;
        case ViewRelation::Ally:  return k_ViewAlly;
        default:                  return k_ViewNone;
        }
    }

    auto HasChannel(const ViewOther& other, ViewChannel channel) -> bool
    {
        return (other.Channels & static_cast<std::uint8_t>(channel)) != 0;
    }

    // Marker of a player: a cross at its middle and a ring at its feet.
    auto AppendMarker(std::vector<Vertex>& vertices, const Vec3& feet, const Color& color) -> void
    {
        AppendCross(vertices, Raised(feet, k_MarkerLift), k_MarkerHalfSize, color);
        AppendCircle(vertices, feet, k_Up, k_RingRadius, k_RingSegments, color);
    }

    // note: An enemy never has the Hud channel, so a Hud row without Seen is an ally.
    auto AppendOther(std::vector<Vertex>& vertices, const View& view, const ViewOther& other, const Vec3& eye) -> void
    {
        if (other.Relation == ViewRelation::Self || other.Channels == 0 || !other.IsAlive) return;

        const bool isSeen{ HasChannel(other, ViewChannel::Seen) };
        const bool isRadar{ HasChannel(other, ViewChannel::Radar) };
        const bool isHud{ HasChannel(other, ViewChannel::Hud) };

        const Color color{ ColorOfRelation(other.Relation) };

        // note: The View has no head position of another player, so the line ends at its Position (the feet).
        if (isSeen)
        {
            if (view.Head.HasBody) AppendLine(vertices, eye, other.Position, color);
            AppendMarker(vertices, other.Position, color);
        }
        else if (isHud)
        {
            AppendMarker(vertices, other.Position, other.Relation == ViewRelation::Ally ? k_ViewAllyDim : color);
        }

        if (isRadar)
        {
            AppendCircle(vertices, other.Position, k_Up, k_RingRadius, k_RingSegments, k_ViewRadar);

            if (other.Elevation != 0)
            {
                const float length{ k_ElevationArrow * static_cast<float>(other.Elevation) };
                AppendArrow(vertices, other.Position, Raised(other.Position, length), k_ViewRadar, false);
            }
        }
    }

    auto AppendFlag(std::vector<Vertex>& vertices, const ViewFlag& flag, const Color& color) -> void
    {
        if (IsZero(flag.Position) && IsZero(flag.StandPosition)) return;

        AppendCross(vertices, Raised(flag.Position, k_MarkerLift), k_FlagHalfSize, color);

        if (flag.State != FlagState::Home && !IsZero(flag.StandPosition))
        {
            AppendArrow(vertices, flag.Position, flag.StandPosition, color, true);
        }

        if (flag.State == FlagState::Carried && flag.CarrierRow < k_MaxPlayers)
        {
            AppendCircle(vertices, flag.Position, k_Up, k_CarrierRingRadius, k_RingSegments, k_Selected);
        }
    }

    auto AppendSound(std::vector<Vertex>& vertices, const Vec3& eye, const Vec3& direction, float intensity) -> void
    {
        const float fraction{ std::clamp(intensity, 0.0f, 1.0f) };
        const float length{ k_SoundMinLength + (k_SoundMaxLength - k_SoundMinLength) * fraction };
        const float magnitude{ std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z) };
        if (magnitude < 1e-6f) return;

        const float scale{ length / magnitude };
        AppendArrow(vertices, eye, Vec3{ eye.X + direction.X * scale, eye.Y + direction.Y * scale, eye.Z + direction.Z * scale },
            k_ViewSound, false);
    }

    // The players that the watched one does not have in any channel, in grey, placed from the Tick.
    auto AppendHidden(std::vector<Vertex>& vertices, const View& view, const Tick& tick) -> void
    {
        if (!tick.Players || !tick.Entities) return;

        const std::size_t rows{ (std::min)(tick.Players->Rows.size(), view.Others.size()) };

        for (std::size_t row{ 0 }; row < rows; ++row)
        {
            const Player& player{ tick.Players->Rows[row] };
            const ViewOther& other{ view.Others[row] };

            if (other.Relation == ViewRelation::Self || row == view.Head.PlayerRow) continue;
            if (other.Channels != 0 || !player.IsAlive()) continue;

            const auto* biped{ tick.Entities->Find(player.BipedHandle) };
            if (!biped) continue;

            AppendMarker(vertices, biped->WorldOrigin, k_ViewHidden);
        }
    }
}

namespace DevTools::Viewer::Map::System
{
    auto ViewPass::Upload(ID3D11Device* device, ID3D11DeviceContext* context,
        const View* view, const Tick* tick, std::uint64_t generation) -> void
    {
        if (!device || !context) return;

        const std::uint32_t key{ view ? view->Head.PlayerHandle : k_NoHandle };
        if (m_Gate.IsCurrent(generation, key)) return;

        m_Gate.Mark(generation, key);
        m_VertexCount = 0;

        if (!view) return;

        std::vector<Vertex> vertices{};
        const Vec3 eye{ ViewPositionOf(*view) };

        // note: The Full mode has no cone: it sees every living player without one.
        if (view->Head.HasBody && view->Head.Mode == ViewMode::HumanLike)
        {
            const float halfAngle{ static_cast<float>(k_SightHalfAngle * std::numbers::pi / 180.0) };
            AppendCone(vertices, eye, AimDirectionOf(*view), halfAngle, k_SightRange, k_ConeLines, k_ViewCone);
        }

        for (const ViewOther& other : view->Others)
        {
            AppendOther(vertices, *view, other, eye);
        }

        if (tick) AppendHidden(vertices, *view, *tick);

        // note: Flags[0] is the flag of the team of the watched player, Flags[1] the rival one.
        AppendFlag(vertices, view->Flags[0], k_ViewAlly);
        AppendFlag(vertices, view->Flags[1], k_ViewEnemy);

        for (std::size_t index{ 0 }; index < view->SoundCount && index < view->Sounds.size(); ++index)
        {
            AppendSound(vertices, eye, view->Sounds[index].Direction, view->Sounds[index].Intensity);
        }

        if (vertices.empty()) return;

        const UINT needed{ static_cast<UINT>(vertices.size()) };

        if (!Engine::Render::System::GrowDynamicVertexBuffer(device, needed,
            k_InitialCapacity, m_VertexBuffer, m_Capacity, "[ViewPass]", m_Log))
        {
            return;
        }

        if (!Engine::Render::System::UploadDynamicVertices(context, m_VertexBuffer.Get(),
            vertices, "[ViewPass]", m_Log))
        {
            return;
        }

        m_VertexCount = needed;
    }

    auto ViewPass::Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void
    {
        if (!context || !m_VertexBuffer || m_VertexCount == 0) return;

        pipeline.Bind(context, VertexLayout::Colored);

        Engine::Render::System::DrawVertexBuffer(context, m_VertexBuffer.Get(),
            m_VertexCount, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    auto ViewPass::Release() -> void
    {
        m_VertexBuffer.Reset();
        m_Capacity = 0;
        m_VertexCount = 0;

        m_Gate.Reset();
    }
}