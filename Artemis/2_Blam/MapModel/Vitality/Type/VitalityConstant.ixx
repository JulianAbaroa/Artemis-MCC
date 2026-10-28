export module Resolved.Vitality.Type:Constant;

import std;

export namespace Resolved::Vitality::Type::Constant
{
    // Instant response flag "Kills Object".
    constexpr std::uint32_t k_FlagKillsObject{ 1u << 0 };

    // Instant response flag "Destroys Object".
    constexpr std::uint32_t k_FlagDestroysObject{ 1u << 7 };

    // Instant response flag "Kills Object (No Player Solo)".
    constexpr std::uint32_t k_FlagKillsObjectNoSolo{ 1u << 10 };

    // Damage section flag "Headshot".
    constexpr std::uint32_t k_FlagHeadshot{ 1u << 4 };
}