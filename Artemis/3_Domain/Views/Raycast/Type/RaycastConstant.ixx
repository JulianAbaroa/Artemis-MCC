export module Views.Raycast.Type:Constant;

import std;

export namespace Views::Raycast::Type::Constant
{
    constexpr float k_FallbackAimRange{ 50.0f };
    constexpr float k_MaxRayRange{ 150.0f };

    // Gathers in a row with the same key before an object counts as at rest and may go to the fixed tree (D300).
    // note: Counted in gathers. Today Self gathers every tick, with RS.2 it will be every decision.
    constexpr std::size_t k_RestGathers{ 8 };

    // Gathers since the last build before the fixed tree is built again for new objects at rest or for objects that left it (D300).
    // note: Counted in gathers. It stays as the least spacing between two builds (D301). Today Self gathers every tick, with RS.2 it will be every decision.
    constexpr std::size_t k_FixedRebuildGathers{ 32 };

    // Objects waiting (at rest and not in the fixed tree, plus handles filtered out of it) that make a build due once the spacing passed (D301).
    constexpr std::size_t k_FixedRebuildPending{ 16 };

    // Gathers that any waiting object may wait before a build is due, once the spacing passed (D301).
    constexpr std::size_t k_FixedRebuildMaxGathers{ 512 };

    // Largest ratio between the area of the inner nodes of the MOBILE tree after a refit and the area it had when it was built.
    // A refit that ends above it is thrown away and the tree is built again (D302).
    // note: The sweep of RS.1d over three films (R of 1.25 to infinity) gave the same rays and the same cost per ray for all of them: the
    // area only jumps when objects are replaced or teleported, and then it is far above any R. 2 builds again those few times and
    // never in between. The tree is refitted at every gather with the same pieces: with RS.2 (a gather per decision) the objects move
    // more between gathers, so recalibrate it.
    constexpr float k_MobileRebuildArea{ 2.0f };
}