export module Blam.Layout.Game.Type:Offset;

import std;

export namespace Blam::Layout::Game::Type::Offset
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

    // Round of the match in the game engine globals: index (int16, 0..) and state (uint8: 1 in play, 2 ended, 3 between rounds) (A14).
    constexpr std::uintptr_t k_GameEngineRoundIndex{ 0xDA };
    constexpr std::uintptr_t k_GameEngineRoundState{ 0xD8 };

    // Team mapping in the game engine globals: one byte per side, the team that plays that side (0xFF unused), k_MaxTeams bytes (A14).
    constexpr std::uintptr_t k_GameEngineTeamMapping{ 0x1C };

    // Score of each team (int16) in the game engine globals, by team, one every k_TeamScoreStride bytes (A14).
    constexpr std::uintptr_t k_GameEngineTeamScoreRound{ 0x32F0 };
    constexpr std::uintptr_t k_GameEngineTeamScoreTotal{ 0x32F4 };
    constexpr std::uintptr_t k_TeamScoreStride{ 0x8 };

    // Fields of an element of the megalo objects region (D250). The state and the timer are raw engine values.
    // note: The timer has 5 + the allies in the zone in its high byte while it runs (0 paused) and the time in 1/1200 s in the low 24 bits (A14).
    constexpr std::uintptr_t k_MegaloObjectHandle{ 0x38 };
    constexpr std::uintptr_t k_MegaloObjectState{ 0x50 };
    constexpr std::uintptr_t k_MegaloObjectResetTimer{ 0x70 };

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