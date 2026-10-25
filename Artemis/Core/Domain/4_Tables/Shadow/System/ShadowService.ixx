export module Tables.Shadow.System:Shadow;

import :PlayerTable;
import :Interaction;
import :Game;

import Common.Log.Type;
import Common.Log.System;
import Platform.Memory.System;
import Platform.Capture.State;
import Tables.Player.State;
import Tables.Interaction.State;
import Tables.Game.State;
import std;

export namespace Tables::Shadow::System
{
    // Runs the old live reads of the players, interactions and game next to the new ones that read the snapshot, and compares their tables.
    // note: Temporary (M4a, D188). M4b deletes it. The copies write their own stores and log nothing.
    class ShadowService
    {
    private:
        using Log = Common::Log::System::Log;
        using LogSink = Common::Log::Type::LogSink;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using CaptureStore = Platform::Capture::State::CaptureStore;

        using PlayerTableStore = Tables::Player::State::PlayerTableStore;
        using InteractionStore = Tables::Interaction::State::InteractionStore;
        using GameStore = Tables::Game::State::GameStore;

    public:
        // param log: The real one, for the differences and the summary.
        // param playerStore: The real stores, the ones the tables of the snapshot go to. They are only read.
        ShadowService(Log& log,
            MemoryReaderService& memoryReaderService,
            CaptureStore& captureStore,
            const PlayerTableStore& playerStore,
            const InteractionStore& interactionStore,
            const GameStore& gameStore);
        ~ShadowService() = default;

        // Runs the old live reads of players, interactions and game and compares their tables with the ones built from the
        // Snapshot, field by field.
        // note: Must run inside the read window, after the real services. Temporary (M4a, D188).
        auto Check(std::uint64_t tick) -> void;

        // Logs how many ticks were compared and resets the counters and the stores of the copies.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        const PlayerTableStore& m_PlayerStore;
        const InteractionStore& m_InteractionStore;
        const GameStore& m_GameStore;

        std::unique_ptr<LogSink> m_MuteSink;
        Log m_MuteLog;

        PlayerTableStore m_ShadowPlayerStore{};
        InteractionStore m_ShadowInteractionStore{};
        GameStore m_ShadowGameStore{};

        ShadowPlayerTableService m_ShadowPlayerService;
        ShadowInteractionService m_ShadowInteractionService;
        ShadowGameService m_ShadowGameService;

        std::uint64_t m_TicksCompared{ 0 };
        std::uint64_t m_TicksWithDifferences{ 0 };
        std::size_t m_LoggedDifferences{ 0 };
        bool m_IsOverflowLogged{ false };

        // Logs one difference, only the first ones of a game.
        // param id: Handle of the player, index of the local player, or 0 for the game.
        auto ReportDifference(std::uint64_t tick, std::string_view table, std::uint32_t id, std::string_view field) -> void;

        // return: Whether the tables of the real and the copy differ in anything.
        auto ComparePlayers(std::uint64_t tick) -> bool;
        auto CompareInteractions(std::uint64_t tick) -> bool;
        auto CompareGame(std::uint64_t tick) -> bool;
    };
}