export module Tables.Object.Type:Placement;

import Common.Semantic.Type;
import std;

namespace
{
    using Physics = Common::Semantic::Type::Physics;
}

export namespace Tables::Object::Type::Placement
{
    // Games a placement spawns in, by the symmetry of the game variant.
    enum class Symmetry : std::uint8_t
    {
        None = 0,
        Symmetric = 1,
        Asymmetric = 2,
        Both = 3,
    };

    // Forge settings of an object placed by the map variant.
    // note: Read from the placement that obje 0x1C points to. Team and zone shape stay in Multiplayer (block of the datum).
    struct Placement
    {
        // Index of the placement in the map variant.
        std::uint16_t Index{};

        // note: Normal is the only dynamic one. Fixed and Phased do not move and keep it after a pick up (D80).
        Physics Physics{};
        Symmetry Symmetry{};
        bool IsPlacedAtStart{};
        bool IsGameSpecific{};
        std::int8_t SpawnSequence{};
        std::uint8_t SpawnTime{};

        // note: 0xFF = default color.
        std::uint8_t Color{};
    };
}