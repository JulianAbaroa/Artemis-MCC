export module Common.Engine.Type;

import std;

export namespace Common::Engine::Type
{
    // Handle of no datum. The engine writes -1 (NONE).
    inline constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };

    // Tick of something that has not happened. The engine writes -1 (NONE).
    inline constexpr std::uint32_t k_NoTick{ 0xFFFFFFFF };

    // Low half of a handle: the index of the datum in its table. The high half is the salt.
    inline constexpr std::uint32_t k_HandleIndexMask{ 0xFFFF };

    // Players the engine tracks.
    // note: Assumed from the maximum players of a match. Not read from the engine.
    inline constexpr std::size_t k_MaxPlayers{ 16 };

    // Characters of a gamertag, terminator included.
    inline constexpr std::size_t k_GamerTagLength{ 16 };
}