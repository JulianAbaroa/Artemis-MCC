#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Log.Type;
import Common.Log.System;
import Common.Math.Type;
import Common.Team.Type;
import Contract.Bridge;
import Datum.Object.Type;
import Egocentric.Layer;
import Environment.Layer;
import Export.Layer;
import Export.Tick.Type;
import Layout.Player.Type;
import Layout.Snapshot.Type;
import Map.Layer;
import Map.Reader.Type;
import Relations.Layer;
import Resolved.Layer;
import Resolved.Definitions.Type;
import Tables.Layer;
import Tables.Player.Type;
import Tests.Support.Blam;
import Tests.Support.Log;
import std;

// The pure chain of M5b (D198, D200): Tables, Relations, Environment, Egocentric and Export built and run without the DLL, from
// Snapshots made by hand and a tag index written by hand. No .map, no MCC, no Platform and no Service.

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::System::Log;
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;
    using Export::Tick::Type::Tick;
    using Layout::Snapshot::Type::RawObject;
    using Layout::Snapshot::Type::RegionId;
    using Layout::Snapshot::Type::RegionSize;
    using Layout::Snapshot::Type::Snapshot;
    using Platform::Bridge::Type::BridgeTick;
    using Resolved::Definitions::Type::Object::ObjectKind;
    using Tables::Player::Type::Alive::ConnectionState;
    using Tests::Support::Blam::SyntheticTag;
    using Tests::Support::Blam::Fill;
    using Tests::Support::Blam::MakeHandle;
    using Tests::Support::Blam::Put;
    using Tests::Support::Blam::CaptureRegion;
    using Tests::Support::Log::CollectingSink;

    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_NoTick;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_MaxPlayers;
    using Map::Reader::Type::Magic::Tag::k_Bipd;

    namespace ObjeOffset = Datum::Object::Type::Obje::Offset;
    namespace UnitOffset = Datum::Object::Type::Unit::Offset;
    namespace PlayerOffset = Layout::Player::Type::Offset;
    namespace PlayerMapping = Layout::Player::Type::Mapping;

    using Layout::Player::Type::Size::k_Base;

    // Bytes of every hand made datum: more than the last field of a biped.
    constexpr std::size_t k_DatumBytes{ 0x1000 };

    const std::array<std::uint32_t, 1> k_Groups{ k_Bipd };

    constexpr std::string_view k_SpartanName{ "objects\\characters\\spartans\\spartans" };

    const std::array<SyntheticTag, 1> k_Tags{ {
        { .Group = 0, .Salt = 0xE000, .Name = k_SpartanName },
    } };

    const std::uint32_t k_SpartanTag{ MakeHandle(0xE000, 0) };
    const std::uint32_t k_BipedHandle{ MakeHandle(0xEC01, 3) };
    const std::uint32_t k_PlayerHandle{ MakeHandle(0xE5A1, 2) };

    // Services that write "Cleanup completed." when their layer is cleaned up, with the layer that owns them.
    const std::array<std::string_view, 22> k_CleanupTags{
        "[ClassifierService]", "[ObjectGraphService]",
        "[VitalityDetailsService]", "[ZoneDetailsService]", "[TeleporterDetailsService]", "[LiftDetailsService]",
        "[BarrierDetailsService]", "[EntitiesService]", "[PlayersService]", "[CollidableService]", "[AimService]",
        "[SelfService]", "[AffordanceService]", "[RaycastService]",
        "[ObjectTableService]", "[PlayerTableService]", "[InteractionService]",
        "[MapLoaderService]", "[MapBuilderService]",
        "[WorldBuilder]", "[DefinitionsBuilder]", "[VitalityBuilder]",
    };

    // return: A biped of the spartan tag whose four damage slots are empty, at (10, 20, 30) looking along +Y with +Z up.
    auto MakeBiped() -> RawObject
    {
        RawObject biped{};
        biped.Handle = k_BipedHandle;
        biped.Address = 0x7FF100000000 + (static_cast<std::uintptr_t>(k_BipedHandle & 0xFFFF) * k_DatumBytes);
        biped.KindId = static_cast<std::uint8_t>(ObjectKind::Biped);
        biped.Bytes.assign(k_DatumBytes, 0);

        Put(biped.Bytes, ObjeOffset::k_TagHandle, k_SpartanTag);
        Put(biped.Bytes, ObjeOffset::k_ParentHandle, k_NoHandle);
        Put(biped.Bytes, ObjeOffset::k_VariantPlacementIndex, std::uint16_t{ 0xFFFF });
        Put(biped.Bytes, ObjeOffset::k_Origin, Vec3{ 10.0f, 20.0f, 30.0f });
        Put(biped.Bytes, ObjeOffset::k_Forward, Vec3{ 0.0f, 1.0f, 0.0f });
        Put(biped.Bytes, ObjeOffset::k_Up, Vec3{ 0.0f, 0.0f, 1.0f });

        const std::size_t slotStride{ UnitOffset::k_Damage1Tick - UnitOffset::k_Damage0Tick };
        for (std::size_t slot{ 0 }; slot < 4; ++slot)
        {
            Put(biped.Bytes, UnitOffset::k_Damage0Tick + (slot * slotStride), k_NoTick);
        }

        return biped;
    }

    // return: A snapshot of a running game with the object table found, the biped and a connected player that owns it.
    auto WithBipedAndPlayer() -> Snapshot
    {
        Snapshot snapshot{};
        snapshot.HasThreadContext = true;
        snapshot.HasObjectTable = true;
        snapshot.RawObjects.emplace(k_BipedHandle, MakeBiped());

        std::vector<std::uint8_t>& players{ CaptureRegion(snapshot, RegionId::PlayerArray, RegionSize(RegionId::PlayerArray)) };
        const std::size_t entry{ 2 * k_Base };
        Put(players, entry + PlayerOffset::k_Handle, std::uint16_t{ 0xE5A1 });
        Put(players, entry + PlayerOffset::k_ConnectionState, ConnectionState::Connected);
        Put(players, entry + PlayerOffset::k_Team, Team::Blue);
        Put(players, entry + PlayerOffset::k_AliveBipedHandle, k_BipedHandle);
        Put(players, entry + PlayerOffset::k_CurrentBipedHandle, k_BipedHandle);
        Put(players, entry + PlayerOffset::k_DeadBipedHandle, k_NoHandle);

        // The player mapping with no player bound to any input user nor output view.
        std::vector<std::uint8_t>& mapping{ CaptureRegion(snapshot, RegionId::PlayerMapping, RegionSize(RegionId::PlayerMapping)) };
        for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            Put(mapping, PlayerMapping::k_PlayerInputUser + (index * PlayerMapping::k_EntryStride), std::int32_t{ -1 });
        }
        for (std::size_t view{ 0 }; view < k_MaxLocalUsers; ++view)
        {
            Put(mapping, PlayerMapping::k_OutputUserPlayer + (view * PlayerMapping::k_EntryStride), k_NoHandle);
        }

        return snapshot;
    }

    // The seven layers of the pure chain over one Log and a tag index written by hand (D197).
    class PipelineTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            Fill(m_Map.m_TagIndexStore, k_Groups, k_Tags);
            m_Map.m_FileStore.SetLoaded(true);
        }

        // Runs the services in the order of AIThread::ExecuteTick and assembles the Tick as Runtime::Thread does.
        auto RunTick(const Snapshot& snapshot, std::uint64_t generation) -> std::shared_ptr<const Tick>
        {
            m_Tables.m_ObjectService.UpdateObjectTable(snapshot);
            m_Tables.m_PlayerService.UpdatePlayerTable(snapshot);
            m_Tables.m_InteractionService.UpdateInteractionTable(snapshot);
            m_Tables.m_GameService.Update(snapshot);

            m_Relations.m_ClassifierService.UpdateClassification();
            m_Relations.m_ObjectGraphService.UpdateGraph();

            m_Environment.m_VitalityDetailsService.Update();
            m_Environment.m_ZoneDetailsService.Update();
            m_Environment.m_TeleporterDetailsService.Update();
            m_Environment.m_LiftDetailsService.Update();
            m_Environment.m_BarrierDetailsService.Update();
            m_Environment.m_EntitiesService.Update();
            m_Environment.m_PlayersService.Update();
            m_Environment.m_CollidableService.Update(false);
            m_Environment.m_AimService.Update();
            m_Environment.m_EventsService.Update();
            m_Environment.m_MatchService.Update();

            m_Egocentric.m_SelfService.Update();
            m_Egocentric.m_AffordanceService.Update();
            m_Egocentric.m_RaycastService.Update();

            m_Export.m_TickService.Assemble(generation);
            return m_Export.m_TickStore.Acquire();
        }

        auto Flatten(const Tick& tick) const -> std::unique_ptr<BridgeTick>
        {
            auto flat{ std::make_unique<BridgeTick>() };
            m_Export.m_FlatService.Flatten(tick, *flat);
            return flat;
        }

        // Cleans up the seven layers in the order Core registers them (D200).
        auto CleanupAll() -> void
        {
            m_Relations.Cleanup();
            m_Environment.Cleanup();
            m_Egocentric.Cleanup();
            m_Export.Cleanup();
            m_Tables.Cleanup();
            m_Map.Cleanup();
            m_Resolved.Cleanup();
        }

        auto CountOfCleanup(std::string_view tag) const -> std::size_t
        {
            return static_cast<std::size_t>(std::ranges::count_if(m_Sink.m_Messages, [tag](const auto& message) {
                return message.Level == LogLevel::Info && message.Tag == tag && message.Message == "Cleanup completed.";
            }));
        }

        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        Map::Layer m_Map{ m_Log };
        Resolved::Layer m_Resolved{ m_Log, m_Map };
        Tables::Layer m_Tables{ m_Log, m_Resolved };
        Relations::Layer m_Relations{ m_Log, m_Resolved, m_Tables };
        Environment::Layer m_Environment{ m_Log, m_Resolved, m_Tables, m_Relations };
        Egocentric::Layer m_Egocentric{ m_Log, m_Resolved, m_Tables, m_Relations, m_Environment };
        Export::Layer m_Export{ m_Resolved, m_Tables, m_Relations, m_Environment, m_Egocentric };
    };
}

TEST_F(PipelineTests, PipelineBuildsWithoutTheDll)
{
    // Nothing ran yet: no layer published anything and nothing was logged.
    EXPECT_EQ(m_Export.m_TickStore.Acquire(), nullptr);
    EXPECT_EQ(m_Tables.m_ObjectStore.Acquire(), nullptr);
    EXPECT_EQ(m_Relations.m_ClassifierStore.Acquire(), nullptr);
    EXPECT_EQ(m_Environment.m_PlayersStore.Acquire(), nullptr);
    EXPECT_EQ(m_Egocentric.m_SelfStore.Acquire(), nullptr);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Error), 0u);
}

TEST_F(PipelineTests, EmptySnapshotGivesAnEmptyBridgeTick)
{
    const auto tick{ this->RunTick(Snapshot{}, 1) };
    ASSERT_NE(tick, nullptr);
    EXPECT_EQ(tick->Generation, 1u);

    const auto flat{ this->Flatten(*tick) };
    EXPECT_EQ(flat->Header.PlayerCount, 0u);
    EXPECT_EQ(flat->Header.EntityCount, 0u);
    EXPECT_EQ(flat->Header.EventCount, 0u);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Error), 0u);
}

TEST_F(PipelineTests, BipedAndPlayerReachTheBridgeTick)
{
    const auto tick{ this->RunTick(WithBipedAndPlayer(), 7) };
    ASSERT_NE(tick, nullptr);

    const auto flat{ this->Flatten(*tick) };
    EXPECT_EQ(flat->Header.Generation, 7u);

    ASSERT_EQ(flat->Header.PlayerCount, 1u);
    EXPECT_EQ(flat->Players[0].Handle, k_PlayerHandle);
    EXPECT_EQ(flat->Players[0].BipedHandle, k_BipedHandle);

    ASSERT_GE(flat->Header.EntityCount, 1u);
    const auto entities{ std::span<const Platform::Bridge::Type::BridgeEntity>{ flat->Entities }.first(flat->Header.EntityCount) };
    EXPECT_TRUE(std::ranges::any_of(entities, [](const auto& entity) { return entity.Handle == k_BipedHandle; }));
}

TEST_F(PipelineTests, CleanupForgetsTheTick)
{
    const auto tick{ this->RunTick(WithBipedAndPlayer(), 1) };
    ASSERT_NE(tick, nullptr);
    ASSERT_EQ(this->Flatten(*tick)->Header.PlayerCount, 1u);

    this->CleanupAll();

    EXPECT_EQ(m_Export.m_TickStore.Acquire(), nullptr);

    const auto empty{ this->RunTick(Snapshot{}, 2) };
    ASSERT_NE(empty, nullptr);
    const auto flat{ this->Flatten(*empty) };
    EXPECT_EQ(flat->Header.PlayerCount, 0u);
    EXPECT_EQ(flat->Header.EntityCount, 0u);
    EXPECT_EQ(flat->Header.EventCount, 0u);

    // One line per service that writes it today (22), none from Export.
    for (const std::string_view tag : k_CleanupTags)
    {
        EXPECT_EQ(this->CountOfCleanup(tag), 1u) << tag;
    }
    const auto total{ std::ranges::count_if(m_Sink.m_Messages, [](const auto& message) { return message.Message == "Cleanup completed."; }) };
    EXPECT_EQ(static_cast<std::size_t>(total), k_CleanupTags.size());
}