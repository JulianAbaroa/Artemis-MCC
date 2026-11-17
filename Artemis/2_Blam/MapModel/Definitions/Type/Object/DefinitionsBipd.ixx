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

        // Height of the camera above the origin of the biped when it stands and when it is crouched.
        float StandingCameraHeight{};
        float CrouchingCameraHeight{};

        // Node of the skeleton that holds the head, -1 if the tag has none.
        std::int16_t HeadNodeIndex{ -1 };
    };
}