export module Layout.Player.Type:Mapping;

import std;

// Player mapping globals of the engine, *(thread context + 0x158) (A9_SplitScreen.txt sec. 11.1).
// note: An input user is a local player of the split-screen (the index of the host input). An output user is a view.
export namespace Layout::Player::Type::Mapping
{
    // Input user of every player (int32 by table index), or -1 when the player is not local.
    constexpr std::uintptr_t k_PlayerInputUser{ 0x74 };

    // Player handle of every output user (uint32 by view index), or 0xFFFFFFFF without one.
    constexpr std::uintptr_t k_OutputUserPlayer{ 0xB8 };

    // Bytes of one entry of the arrays above.
    constexpr std::uintptr_t k_EntryStride{ 0x4 };
}