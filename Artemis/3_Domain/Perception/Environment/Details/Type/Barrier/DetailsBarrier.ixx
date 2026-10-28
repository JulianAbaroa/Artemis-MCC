export module Environment.Details.Type:Barrier;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace Environment::Details::Type
{
    // Barrier data of one entity with Function Barrier (D91).
    // note: The kind is the subtype of the entity and the pose is in Entities. Every barrier blocks every projectile.
    // OneWay (from its passable side) and TwoWay let everything else through: bipeds, vehicles and objects.
    struct BarrierDetail
    {
        std::uint32_t Handle{};

        // Direction of travel that goes through a OneWay barrier, world space. Zero for the other subtypes.
        // note: The Forward of the model, or its opposite when the catalog marks the tag facing="reversed" (the ff_one_way_door tags).
        Vec3 PassDirection{};
    };

    struct BarrierDetails : std::unordered_map<std::uint32_t, BarrierDetail>
    {
        using unordered_map::unordered_map;
    };
}