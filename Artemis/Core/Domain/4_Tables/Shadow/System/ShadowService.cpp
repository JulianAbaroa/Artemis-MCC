module Tables.Shadow.System;
import :Shadow;

import Common.Engine.Type;
import Common.Log.Type;
import Tables.Player.Type;
import Tables.Interaction.Type;
import Tables.Game.Type;

namespace
{
    using Common::Log::Type::LogMessage;
    using Common::Log::Type::LogSink;

    using Tables::Player::Type::Alive::AlivePlayer;
    using Tables::Player::Type::Alive::PlayerTable;
    using Tables::Interaction::Type::Alive::AliveInteraction;
    using Tables::Interaction::Type::Alive::LocalInteractions;
    using Tables::Game::Type::Alive::AliveEvent;
    using Tables::Game::Type::Alive::AliveGame;

    using Common::Engine::Type::k_MaxLocalUsers;

    // Differences logged per game, so a broken layout does not flood the log.
    constexpr std::size_t k_MaxLoggedDifferences{ 20 };

    // Sink of the copies: they run the old code, which would repeat the lines of the real services.
    class MuteSink final : public LogSink
    {
    public:
        auto Write(const LogMessage&) -> void override {}
    };

    // return: Whether both values have the same bits. NaN equals NaN and -0 differs from +0.
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    auto SameBits(const T& left, const T& right) -> bool
    {
        return std::memcmp(&left, &right, sizeof(T)) == 0;
    }
}

namespace Tables::Shadow::System
{
    ShadowService::ShadowService(Log& log,
        MemoryReaderService& memoryReaderService,
        CaptureStore& captureStore,
        const PlayerTableStore& playerStore,
        const InteractionStore& interactionStore,
        const GameStore& gameStore) :
        m_Log(log),
        m_PlayerStore(playerStore), m_InteractionStore(interactionStore), m_GameStore(gameStore),
        m_MuteSink(std::make_unique<MuteSink>()), m_MuteLog(*m_MuteSink),
        m_ShadowPlayerService(m_MuteLog, memoryReaderService, captureStore, m_ShadowPlayerStore),
        m_ShadowInteractionService(m_MuteLog, memoryReaderService, captureStore, m_ShadowInteractionStore),
        m_ShadowGameService(m_MuteLog, memoryReaderService, captureStore, m_ShadowGameStore) {}

    auto ShadowService::Check(std::uint64_t tick) -> void
    {
        m_ShadowPlayerService.UpdatePlayerTable();
        m_ShadowInteractionService.UpdateInteractionTable();
        m_ShadowGameService.Update();

        const bool arePlayersDifferent{ this->ComparePlayers(tick) };
        const bool areInteractionsDifferent{ this->CompareInteractions(tick) };
        const bool isGameDifferent{ this->CompareGame(tick) };

        ++m_TicksCompared;
        if (arePlayersDifferent || areInteractionsDifferent || isGameDifferent) ++m_TicksWithDifferences;
    }

    auto ShadowService::ReportDifference(std::uint64_t tick, std::string_view table, std::uint32_t id, std::string_view field) -> void
    {
        if (m_LoggedDifferences >= k_MaxLoggedDifferences)
        {
            if (m_IsOverflowLogged) return;

            m_IsOverflowLogged = true;
            m_Log.Message("[ShadowService] ERROR: More differences after tick {}; only the first {} are logged.", tick, k_MaxLoggedDifferences);
            return;
        }

        ++m_LoggedDifferences;
        m_Log.Message("[ShadowService] ERROR: Tick {}: {} {:08X} {} differs.", tick, table, id, field);
    }

    auto ShadowService::ComparePlayers(std::uint64_t tick) -> bool
    {
        const std::shared_ptr<const PlayerTable> real{ m_PlayerStore.Acquire() };
        const std::shared_ptr<const PlayerTable> copy{ m_ShadowPlayerStore.Acquire() };

        if (!real && !copy) return false;
        if (!real || !copy)
        {
            this->ReportDifference(tick, "Players", 0, "published");
            return true;
        }

        bool hasDifferences{ false };

        auto differs = [&](std::uint32_t handle, std::string_view field) {
            this->ReportDifference(tick, "Players", handle, field);
            hasDifferences = true;
        };

        for (const auto& [handle, player] : *real)
        {
            const auto found{ copy->find(handle) };
            if (found == copy->end())
            {
                differs(handle, "presence");
                continue;
            }

            const AlivePlayer& other{ found->second };

            if (!SameBits(player.Handle, other.Handle)) differs(handle, "Handle");
            if (!SameBits(player.Address, other.Address)) differs(handle, "Address");
            if (!SameBits(player.ConnectionState, other.ConnectionState)) differs(handle, "ConnectionState");
            if (!SameBits(player.Team, other.Team)) differs(handle, "Team");
            if (player.Gamertag != other.Gamertag) differs(handle, "Gamertag");
            if (player.Tag != other.Tag) differs(handle, "Tag");
            if (!SameBits(player.CameraPosition, other.CameraPosition)) differs(handle, "CameraPosition");
            if (!SameBits(player.CameraForward, other.CameraForward)) differs(handle, "CameraForward");
            if (!SameBits(player.AimOffset, other.AimOffset)) differs(handle, "AimOffset");
            if (!SameBits(player.PrimaryWeaponHandle, other.PrimaryWeaponHandle)) differs(handle, "PrimaryWeaponHandle");
            if (!SameBits(player.SecondaryWeaponHandle, other.SecondaryWeaponHandle)) differs(handle, "SecondaryWeaponHandle");
            if (!SameBits(player.ObjectiveHandle, other.ObjectiveHandle)) differs(handle, "ObjectiveHandle");
            if (!SameBits(player.AliveBipedHandle, other.AliveBipedHandle)) differs(handle, "AliveBipedHandle");
            if (!SameBits(player.DeadBipedHandle, other.DeadBipedHandle)) differs(handle, "DeadBipedHandle");
            if (!SameBits(player.CurrentBipedHandle, other.CurrentBipedHandle)) differs(handle, "CurrentBipedHandle");
            if (player.LocalIndex != other.LocalIndex) differs(handle, "LocalIndex");
        }

        for (const auto& [handle, player] : *copy)
        {
            if (!real->contains(handle)) differs(handle, "presence");
        }

        return hasDifferences;
    }

    auto ShadowService::CompareInteractions(std::uint64_t tick) -> bool
    {
        const std::shared_ptr<const LocalInteractions> real{ m_InteractionStore.Acquire() };
        const std::shared_ptr<const LocalInteractions> copy{ m_ShadowInteractionStore.Acquire() };

        if (!real && !copy) return false;
        if (!real || !copy)
        {
            this->ReportDifference(tick, "Interactions", 0, "published");
            return true;
        }

        bool hasDifferences{ false };

        for (std::uint32_t index{ 0 }; index < k_MaxLocalUsers; ++index)
        {
            const AliveInteraction& interaction{ (*real)[index] };
            const AliveInteraction& other{ (*copy)[index] };

            auto differs = [&](std::string_view field) {
                this->ReportDifference(tick, "Interactions", index, field);
                hasDifferences = true;
            };

            if (!SameBits(interaction.PlayerHandle, other.PlayerHandle)) differs("PlayerHandle");
            if (!SameBits(interaction.Kind, other.Kind)) differs("Kind");
            if (!SameBits(interaction.InteractionSlotID, other.InteractionSlotID)) differs("InteractionSlotID");
            if (!SameBits(interaction.TargetObjectHandle, other.TargetObjectHandle)) differs("TargetObjectHandle");
            if (!SameBits(interaction.IsMeleeAvailable, other.IsMeleeAvailable)) differs("IsMeleeAvailable");
            if (!SameBits(interaction.MeleeTargetHandle, other.MeleeTargetHandle)) differs("MeleeTargetHandle");
            if (!SameBits(interaction.IsAimAvailable, other.IsAimAvailable)) differs("IsAimAvailable");
            if (!SameBits(interaction.ModelPart, other.ModelPart)) differs("ModelPart");
            if (!SameBits(interaction.AimTargetHandle, other.AimTargetHandle)) differs("AimTargetHandle");
            if (!SameBits(interaction.AimTargetSlotID, other.AimTargetSlotID)) differs("AimTargetSlotID");
            if (!SameBits(interaction.AimHitLocalPosition, other.AimHitLocalPosition)) differs("AimHitLocalPosition");
        }

        return hasDifferences;
    }

    auto ShadowService::CompareGame(std::uint64_t tick) -> bool
    {
        const std::shared_ptr<const AliveGame> real{ m_GameStore.Acquire() };
        const std::shared_ptr<const AliveGame> copy{ m_ShadowGameStore.Acquire() };

        if (!real && !copy) return false;
        if (!real || !copy)
        {
            this->ReportDifference(tick, "Game", 0, "published");
            return true;
        }

        bool hasDifferences{ false };

        auto differs = [&](std::uint32_t id, std::string_view field) {
            this->ReportDifference(tick, "Game", id, field);
            hasDifferences = true;
        };

        if (!SameBits(real->EngineTick, copy->EngineTick)) differs(0, "EngineTick");
        if (!SameBits(real->Phase, copy->Phase)) differs(0, "Phase");
        if (!SameBits(real->PlayerScores, copy->PlayerScores)) differs(0, "PlayerScores");
        if (!SameBits(real->LastSequence, copy->LastSequence)) differs(0, "LastSequence");
        if (!SameBits(real->DroppedEvents, copy->DroppedEvents)) differs(0, "DroppedEvents");

        if (real->Events.size() != copy->Events.size())
        {
            differs(0, "Events.size");
            return hasDifferences;
        }

        for (std::uint32_t index{ 0 }; index < real->Events.size(); ++index)
        {
            const AliveEvent& event{ real->Events[index] };
            const AliveEvent& other{ copy->Events[index] };

            if (!SameBits(event.Sequence, other.Sequence)) differs(index, "Events.Sequence");
            if (!SameBits(event.GameTick, other.GameTick)) differs(index, "Events.GameTick");
            if (!SameBits(event.Key, other.Key)) differs(index, "Events.Key");
            if (!SameBits(event.CausePlayer, other.CausePlayer)) differs(index, "Events.CausePlayer");
            if (!SameBits(event.CauseObject, other.CauseObject)) differs(index, "Events.CauseObject");
            if (!SameBits(event.EffectPlayer, other.EffectPlayer)) differs(index, "Events.EffectPlayer");
            if (!SameBits(event.EffectObject, other.EffectObject)) differs(index, "Events.EffectObject");
            if (!SameBits(event.DamageReportingType, other.DamageReportingType)) differs(index, "Events.DamageReportingType");
            if (!SameBits(event.Modifier, other.Modifier)) differs(index, "Events.Modifier");
            if (!SameBits(event.Extra, other.Extra)) differs(index, "Events.Extra");
            if (!SameBits(event.Data, other.Data)) differs(index, "Events.Data");
        }

        return hasDifferences;
    }

    auto ShadowService::Cleanup() -> void
    {
        m_Log.Message("[ShadowService] INFO: {} ticks compared, {} with differences.", m_TicksCompared, m_TicksWithDifferences);

        m_TicksCompared = 0;
        m_TicksWithDifferences = 0;
        m_LoggedDifferences = 0;
        m_IsOverflowLogged = false;

        m_ShadowPlayerService.Cleanup();
        m_ShadowInteractionService.Cleanup();
        m_ShadowGameService.Cleanup();
    }
}