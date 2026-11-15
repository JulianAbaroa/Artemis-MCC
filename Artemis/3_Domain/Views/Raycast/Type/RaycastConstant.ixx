export module Views.Raycast.Type:Constant;

import std;

export namespace Views::Raycast::Type::Constant
{
    constexpr float k_FallbackAimRange{ 50.0f };
    constexpr float k_MaxRayRange{ 150.0f };
    constexpr float k_PerceptionRange{ 20.0f };
    constexpr int k_PerceptionRayCount{ 14 };

    // Gathers in a row with the same key before an object counts as at rest and may go to the fixed tree (D300).
    // note: Counted in gathers. Today Self gathers every tick, with RS.2 it will be every decision.
    constexpr std::size_t k_RestGathers{ 8 };

    // Gathers since the last build before the fixed tree is built again for new objects at rest or for objects that left it (D300).
    constexpr std::size_t k_FixedRebuildGathers{ 32 };
}