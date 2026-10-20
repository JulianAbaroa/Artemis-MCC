export module Environment.Aim.Type;

import Common.Math.Type;
import Resolved.World.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Resolved::World::Type::ModelLink::AnchorSource;
}

export namespace Environment::Aim::Type
{
    struct SectionAim
    {
        Vec3 Position{};
        float Radius{};
        AnchorSource AimSource{ AnchorSource::None };
        bool Valid{ false };
    };

    struct Aim
    {
        std::uint32_t Handle{};

        std::vector<SectionAim> Sections{};
    };

    // Aims, by object handle.
    struct Aims : std::unordered_map<std::uint32_t, Aim>
    {
        using unordered_map::unordered_map;
    };
}