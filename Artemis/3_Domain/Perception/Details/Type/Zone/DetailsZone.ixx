export module Perception.Details.Type:Zone;

import Common.ZoneShape.Type;
import std;

namespace
{
    using Common::ZoneShape::Type::ZoneShape;
}

export namespace Perception::Details::Type
{
    // Zone of one entity with HasZone: boundaries, spawn zones, objective spawns and zones, teleporters and any other (D88).
    // note: The function, the team and the pose come from the row of the entity in Entities.
    struct ZoneDetail
    {
        std::uint32_t Handle{};

        // Cylinder or box, measured from the WorldOrigin of the entity along its Forward and Up.
        ZoneShape Shape{};
    };

    struct ZoneDetails : std::unordered_map<std::uint32_t, ZoneDetail>
    {
        using unordered_map::unordered_map;
    };
}