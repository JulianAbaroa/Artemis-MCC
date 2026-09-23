export module Tables.Player.Type:Size;

import std;

export namespace Tables::Player::Type::Size
{
	// Every player entry inside the player table occupies 1168 bytes.
	constexpr std::size_t Base{ 0x490 };
}