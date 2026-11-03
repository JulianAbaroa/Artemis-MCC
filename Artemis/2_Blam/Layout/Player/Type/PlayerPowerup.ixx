export module Blam.Layout.Player.Type:Powerup;

import Common.Semantic.Type;
import Blam.Layout.Game.Type;
import std;

namespace
{
    using Blam::Layout::Game::Type::Offset::k_PowerupCount;
}

// Custom powerup the engine applies to a player, from the game times of its pickups (D263). One place for the rule of the engine.
export namespace Blam::Layout::Player::Type::Powerup
{
    // Powerup active in a tick and the ticks it has left.
    struct ActivePowerupState
    {
        Common::Semantic::Type::Subtype::Powerup Subtype{};
        // Ticks left counting the current one, 0 when there is no powerup.
        std::uint32_t Ticks{};
    };

    // Returns the powerup the engine applies at game time now.
    // param pickups: Game time of the pickup of the red, blue and yellow powerup, 0 for never. durations: Duration of each one in ticks.
    // note: Only the most recent pickup counts: an older powerup that has not expired does not come back when the newer one ends.
    // note: The ticks left are dur - (now - pickup) + 1, active while above 0. A duration of 0 (no usable variant) is never active.
    constexpr auto ActivePowerup(const std::array<std::int32_t, k_PowerupCount>& pickups, std::uint32_t now,
        const std::array<std::uint32_t, k_PowerupCount>& durations) -> ActivePowerupState
    {
        std::size_t latest{ k_PowerupCount };
        for (std::size_t index{ 0 }; index < k_PowerupCount; ++index)
        {
            if (pickups[index] <= 0) continue;
            if (latest == k_PowerupCount || pickups[index] > pickups[latest]) latest = index;
        }
        if (latest == k_PowerupCount || durations[latest] == 0) return {};

        const std::int64_t left{ static_cast<std::int64_t>(durations[latest]) - (static_cast<std::int64_t>(now) - pickups[latest]) + 1 };
        if (left <= 0) return {};

        return { .Subtype = static_cast<Common::Semantic::Type::Subtype::Powerup>(latest + 1), .Ticks = static_cast<std::uint32_t>(left) };
    }
}