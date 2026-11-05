export module Blam.Runtime.Object.Type:Crate;

import Common.ZoneShape.Type;
import std;

namespace
{
    using Common::ZoneShape::Type::ZoneShape;
}

export namespace Blam::Runtime::Object::Type::Crate
{
    namespace Teleport
    {
        enum class Allowed : std::uint8_t
        {
            None = 0x00,
            NoPlayers = 0x01,
            AllowGround = 0x02,
            AllowHeavy = 0x04,
            AllowFlying = 0x08,
            AllowProjctile = 0x10,
        };

        inline Allowed operator|(Allowed a, Allowed b)
        {
            return static_cast<Allowed>(
                static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
        }

        inline auto HasFlag(Allowed mask, Allowed flag) -> bool
        {
            return (static_cast<std::uint8_t>(mask) &
                static_cast<std::uint8_t>(flag)) != 0;
        }

        struct Teleport
        {
            ZoneShape ZoneShape{};
            std::uint8_t Channel{};
            Allowed Allowed{};
        };
    }

    struct Crate
    {
        std::optional<Teleport::Teleport> Teleport{};
    };
}