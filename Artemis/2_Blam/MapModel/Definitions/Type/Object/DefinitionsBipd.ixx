export module Blam.MapModel.Definitions.Type:Bipd;

import :Object;

import std;

export namespace Blam::MapModel::Definitions::Type::Bipd
{
    // Holds the biped definition.
    struct Bipd
    {
        Object::Object Base{};

        // Bit 0 centered at origin (else the origin is at the bottom of the shape), bit 1 spherical shape.
        std::uint32_t PhysicsFlags{};

        float HeightStanding{};
        float HeightCrouching{};
        float PhysicsRadius{};
    };
}