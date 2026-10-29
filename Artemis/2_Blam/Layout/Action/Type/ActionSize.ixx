export module Blam.Layout.Action.Type:Size;

import std;

export namespace Blam::Layout::Action::Type::Size
{
    // Players a simulation update holds actions for, by player table index (A9_SplitScreen.txt sec. 12).
    inline constexpr std::size_t k_MaxActionPlayers{ 16 };

    // Bytes of the action of one player.
    inline constexpr std::size_t k_PlayerActionSize{ 0xA8 };

    // Bytes of the interaction of the action (see k_ActionInteractionOffset).
    inline constexpr std::size_t k_ActionInteractionSize{ 0x24 };
}