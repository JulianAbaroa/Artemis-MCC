export module Layout.Player.Type:Size;

import std;

export namespace Layout::Player::Type::Size
{
    // Bytes every player entry occupies inside the player table.
    constexpr std::size_t k_Base{ 0x490 };

    // Characters of the service tag in a player entry.
    constexpr std::size_t k_TagLength{ 4 };
}