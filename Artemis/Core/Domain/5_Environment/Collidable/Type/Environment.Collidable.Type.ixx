export module Environment.Collidable.Type;

import Common.Math.Type;
import Resolved.World.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Mesh = Resolved::World::Type::Coll::Mesh;
    using ResolvedColl = Resolved::World::Type::Coll::Coll;
    using ResolvedRegionStates = Resolved::World::Type::RegionStates::RegionStates;
}

export namespace Environment::Collidable::Type
{
    struct Context
    {
        const ResolvedColl* Coll{ nullptr };

        const ResolvedRegionStates* States{ nullptr };
        std::uint8_t Variant{};
    };

    struct Collidable
    {
        std::uint32_t Handle{};
        std::string TagName{};

        Vec3 Position{};
        Vec3 Forward{};
        Vec3 Up{};

        Mesh WorldMesh{};

        bool AncestorDead{ false };
        bool HasDestroyedGeometry{ false };
    };
}