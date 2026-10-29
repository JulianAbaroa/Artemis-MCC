export module Perception.Details.Type:Teleporter;

import std;

export namespace Perception::Details::Type
{
    // What a teleporter lets through, from the allowed flags of its multiplayer block.
    enum class TeleporterPass : std::uint8_t
    {
        None = 0x00,
        Players = 0x01,
        GroundVehicles = 0x02,
        HeavyVehicles = 0x04,
        FlyingVehicles = 0x08,
        Projectiles = 0x10,
    };

    constexpr auto operator|(TeleporterPass left, TeleporterPass right) -> TeleporterPass
    {
        return static_cast<TeleporterPass>(static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right));
    }

    // return: True when the set has the pass.
    constexpr auto HasPass(TeleporterPass set, TeleporterPass pass) -> bool
    {
        return (static_cast<std::uint8_t>(set) & static_cast<std::uint8_t>(pass)) != 0;
    }

    // Teleporter data of one entity with Function Teleporter (D90).
    // note: The kind (Sender, Receiver, TwoWay) is the subtype of the entity, the zone is in ZoneDetails and the pose in Entities.
    struct TeleporterDetail
    {
        std::uint32_t Handle{};
        std::uint8_t Channel{};
        TeleporterPass Passes{ TeleporterPass::None };

        // Live exits on the same channel, sorted by handle: the Receivers of a Sender, the other TwoWays of a TwoWay.
        // note: Always empty for a Receiver.
        std::vector<std::uint32_t> Destinations{};
    };

    struct TeleporterDetails : std::unordered_map<std::uint32_t, TeleporterDetail>
    {
        using unordered_map::unordered_map;
    };
}