module Views.Raycast.System;

import Common.Engine.Type;
import Common.Math.Type;
import Blam.MapModel.World.System;
import Views.Raycast.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::World::System::Raycaster;
    using Views::Raycast::Type::PlayerExclusions;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SelfExclusion;
    using Views::Raycast::Type::SightCounters;
    using Views::Raycast::Type::SightPlayer;
    using Views::Raycast::Type::SightPlayers;
    using Views::Raycast::Type::SightSeen;

    using Common::Engine::Type::k_MaxPlayers;
    using Views::Raycast::Type::Sight::k_SightHalfAngle;
    using Views::Raycast::Type::Sight::k_SightRange;

    // Two heads closer than this are the same point: there is no line to cast.
    constexpr float k_SameHeadDistance{ 1.0e-4f };

    // return: Whether the direction to a point at that distance is within the half angle of the aim of the viewer.
    auto IsInCone(const SightPlayer& viewer, const Vec3& toward, float distance) -> bool
    {
        if (distance < k_SameHeadDistance) return true;

        const double aimLength{ std::sqrt(static_cast<double>(viewer.Aim.X) * viewer.Aim.X + static_cast<double>(viewer.Aim.Y) * viewer.Aim.Y
            + static_cast<double>(viewer.Aim.Z) * viewer.Aim.Z) };
        if (aimLength <= 0.0) return false;

        const double dot{ static_cast<double>(viewer.Aim.X) * toward.X + static_cast<double>(viewer.Aim.Y) * toward.Y
            + static_cast<double>(viewer.Aim.Z) * toward.Z };
        return dot / (aimLength * distance) >= std::cos(k_SightHalfAngle * std::numbers::pi / 180.0);
    }
}

namespace Views::Raycast::System
{
    auto ComputeSight(const Raycaster& raycaster, const RaycastScene& scene, const PlayerExclusions& exclusions, const SightPlayers& players,
        SightCounters& counters) -> SightSeen
    {
        SightSeen seen{};
        SelfExclusion both{};

        for (std::size_t first{ 0 }; first < k_MaxPlayers; ++first)
        {
            const SightPlayer& a{ players[first] };
            if (!a.HasHead) continue;

            for (std::size_t second{ first + 1 }; second < k_MaxPlayers; ++second)
            {
                const SightPlayer& b{ players[second] };
                if (!b.HasHead) continue;

                const Vec3 delta{ b.Head.X - a.Head.X, b.Head.Y - a.Head.Y, b.Head.Z - a.Head.Z };
                const float distance{ std::sqrt(delta.X * delta.X + delta.Y * delta.Y + delta.Z * delta.Z) };
                if (distance > k_SightRange) continue;

                const bool aSeesB{ a.IsViewer && IsInCone(a, delta, distance) };
                const bool bSeesA{ b.IsViewer && IsInCone(b, Vec3{ -delta.X, -delta.Y, -delta.Z }, distance) };
                if (!aSeesB && !bSeesA) continue;
                ++counters.Candidates;

                bool isFree{ true };
                if (distance >= k_SameHeadDistance)
                {
                    both.clear();
                    for (const std::size_t row : { first, second })
                    {
                        if (row < exclusions.size()) both.insert(exclusions[row].begin(), exclusions[row].end());
                    }

                    const Vec3 direction{ delta.X / distance, delta.Y / distance, delta.Z / distance };
                    isFree = !CastSceneRay(raycaster, scene, both, a.Head, direction, distance, nullptr).Hit;
                    ++counters.Rays;
                }

                seen[first][second] = aSeesB && isFree;
                seen[second][first] = bSeesA && isFree;
            }
        }

        return seen;
    }
}