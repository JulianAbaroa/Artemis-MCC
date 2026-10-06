export module Common.Team.Type;

import std;

export namespace Common::Team::Type
{
    // Team of a player. The values match the game.
    enum class Team : std::uint8_t
    {
        Red = 0x00,
        Blue = 0x01,
        Green = 0x02,
        Orange = 0x03,
        Purple = 0x04,
        Gold = 0x05,
        Brown = 0x06,
        Pink = 0x07,
        Neutral = 0x08,
    };
}