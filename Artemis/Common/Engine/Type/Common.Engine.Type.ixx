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

    // Bits the salt is shifted by inside a handle.
    inline constexpr std::uint32_t k_SaltShift{ 16 };

    // Players the engine tracks.
    // note: Assumed from the maximum players of a match. Not read from the engine.
    inline constexpr std::size_t k_MaxPlayers{ 16 };

    // Local players (split-screen) the engine supports.
    inline constexpr std::size_t k_MaxLocalUsers{ 4 };

    // Slots of the object table: Artemis scans this many objects per tick and the bridge exports this many entities.
    // note: Assumed from the object limit of the engine. Not read from it.
    inline constexpr std::uint32_t k_MaxObjects{ 1024 };

    // Characters of a gamertag, terminator included.
    inline constexpr std::size_t k_GamerTagLength{ 16 };
}