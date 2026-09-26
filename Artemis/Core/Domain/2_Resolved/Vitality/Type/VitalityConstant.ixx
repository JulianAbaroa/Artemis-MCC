export module Resolved.Vitality.Type:Constant;

import std;

export namespace Resolved::Vitality::Type::Constant
{
    // "Kills Object"
    constexpr std::uint32_t k_FlagKillsObject{ (1u << 0) };

    // "Kills Object (No Player Solo)"
    constexpr std::uint32_t k_FlagKillsObjectNoSolo{ (1u << 10) };

    // "Headshot"
    constexpr std::uint32_t k_FlagHeadshot{ (1u << 4) };
}