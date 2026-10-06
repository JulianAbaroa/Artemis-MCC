export module Common.ZoneShape.Type;

import std;

export namespace Common::ZoneShape::Type
{
    // Shape of a zone. The values match the game.
    enum class Kind : std::uint8_t
    {
        None = 0x00,
        // 0x01 is the sphere. It is unused.
        Cylinder = 0x02,
        Box = 0x03,
    };

    // Dimensions of a zone. Which fields apply depends on the kind.
    struct ZoneShape
    {
        // The radius, or the width for a box.
        float Radius{};
        float Length{};
        float Top{};
        float Bottom{};
        Kind Kind{ Kind::None };
    };
}