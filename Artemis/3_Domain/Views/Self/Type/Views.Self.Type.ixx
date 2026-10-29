export module Views.Self.Type;

import Common.Math.Type;
import Common.Team.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;
}

export namespace Views::Self::Type
{
    // Local player (split-screen input user) the agent is.
    // note: One agent per process until B5 Views gives every local player its own (A9 M3).
    inline constexpr std::uint8_t k_SelfLocalIndex{ 0 };

    struct Self
    {
        std::uint32_t Handle{ 0xFFFFFFFF };
        std::uint32_t BipedHandle{ 0xFFFFFFFF };

        Vec3 Position{};
        Vec3 Forward{};
        Vec3 Right{};
        Vec3 Up{};

        Team Team{};
        bool IsAlive{ false };

        auto DistanceTo(const Vec3& point) const -> std::optional<float>
        {
            if (!IsAlive) return std::nullopt;

            const float dx = point.X - Position.X;
            const float dy = point.Y - Position.Y;
            const float dz = point.Z - Position.Z;

            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }
    };
}