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
    };
}