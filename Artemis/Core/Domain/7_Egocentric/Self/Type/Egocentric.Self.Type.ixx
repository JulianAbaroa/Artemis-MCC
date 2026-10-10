export module Egocentric.Self.Type;

import Common.Math.Type;
import Common.Team.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Team = Common::Team::Type::Team;
}

export namespace Egocentric::Self::Type
{
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