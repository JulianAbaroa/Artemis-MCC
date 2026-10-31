export module Perception.Details.Type:Explosive;

import std;

export namespace Perception::Details::Type
{
    // Explosion of one entity with Function Explosive (fusion coil, land mine, plasma battery, propane tank), from its tags (D259).
    // note: The life is the Vitality of the entity and the pose is in Entities. The explosion is the damage effect of the instant
    // response with the lowest threshold of the model that has one (the one that fires when the object is destroyed).
    // Every field is 0 when the tags have no explosion.
    struct ExplosiveDetail
    {
        std::uint32_t Handle{};

        // Full damage up to RadiusMin, none past RadiusMax, in world units.
        float RadiusMin{};
        float RadiusMax{};

        // Damage points at the center, before the material of the victim (upper bound of the damage effect).
        float Damage{};
    };

    struct ExplosiveDetails : std::unordered_map<std::uint32_t, ExplosiveDetail>
    {
        using unordered_map::unordered_map;
    };
}