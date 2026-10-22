export module Tables.Game.Type:Offset;

import std;

export namespace Tables::Game::Type::Offset
{
    // Phase of the match in the game engine globals (uint32) (D103).
    // note: The game engine globals are at *(thread context + k_ThreadContextGameEngineOffset).
    constexpr std::uintptr_t k_GameEnginePhase{ 0x0000 };

    // Bits of the raw value that hold the phase. Bit 3 is something else: set in CTF (12 and 13), clear in Slayer (4 and 5) (A14).
    constexpr std::uint32_t k_PhaseMask{ 0x7 };

    // Values of the phase: playing, and after the end of a round or of the match. Pregame is not measured yet.
    constexpr std::uint32_t k_PhaseInGame{ 4 };
    constexpr std::uint32_t k_PhaseEnded{ 5 };

    // Score of each player (int16) in the game engine globals, by player table index, one every k_PlayerScoreStride bytes.
    // note: Measured in Slayer (A5). In CTF it stays 0 after a capture (A14): the layout may depend on the game mode.
    constexpr std::uintptr_t k_GameEnginePlayerScores{ 0x3270 };
    constexpr std::uintptr_t k_PlayerScoreStride{ 0x8 };

    // Event ring in the event globals (*(thread context + k_ThreadContextEventGlobalsOffset)) (D104).
    // note: The entry of a sequence is at k_EventRing + (sequence % k_EventRingSlots) * k_EventEntrySize.
    constexpr std::uintptr_t k_EventRing{ 0xDB28 };
    constexpr std::uint32_t k_EventRingSlots{ 0x100 };
    constexpr std::uintptr_t k_EventEntrySize{ 0x30 };

    // Fields of an entry of the event ring.
    constexpr std::uintptr_t k_EntryKey{ 0x00 };
    constexpr std::uintptr_t k_EntryData{ 0x08 };
    constexpr std::uintptr_t k_EntrySequence{ 0x24 };
    constexpr std::uintptr_t k_EntryGameTick{ 0x28 };

    // Bytes of the event data of an entry.
    constexpr std::size_t k_EventDataSize{ 0x1C };

    // Fields of the event data. Players and objects are full handles.
    constexpr std::uintptr_t k_DataCausePlayer{ 0x00 };
    constexpr std::uintptr_t k_DataCauseObject{ 0x04 };
    constexpr std::uintptr_t k_DataEffectPlayer{ 0x08 };
    constexpr std::uintptr_t k_DataEffectObject{ 0x0C };
    constexpr std::uintptr_t k_DataDamageReportingType{ 0x12 };
    constexpr std::uintptr_t k_DataModifier{ 0x13 };
    constexpr std::uintptr_t k_DataExtra{ 0x14 };

    // Values of the modifier of a kill.
    constexpr std::uint8_t k_ModifierHeadshot{ 0x01 };
    constexpr std::uint8_t k_ModifierStuck{ 0x04 };
}