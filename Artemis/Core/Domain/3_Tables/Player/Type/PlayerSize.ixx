export module Tables.Player.Type:Size;

import std;

export namespace Tables::Player::Type::Size
{
    // Bytes every player entry occupies inside the player table.
    constexpr std::size_t k_Base{ 0x490 };
}