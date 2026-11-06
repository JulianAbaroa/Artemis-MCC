#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Log.Type;
import Contract.Bridge.Type;
import Bridge.Codec.Pipeline.System;
import Bridge.Codec.Tick.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import Blam.Runtime.Object.Type;
import Blam.Layout.Snapshot.Type;
import Blam.MapModel.Definitions.Type;
import Tests.Support.Blam;
import Tests.Support.Pipeline;
import std;

// The pure chain of M5b (D198, D200): Tables, Environment (with Relations), Egocentric and Export built and run without the DLL, from
// Snapshots made by hand and a tag index written by hand. No .map, no MCC, no Platform and no Service.

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Log::Type::LogLevel;
    using Bridge::Codec::Tick::Type::Tick;
    using Blam::Layout::Snapshot::Type::RegionId;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Tests::Support::Blam::MakeHandle;
    using Tests::Support::Blam::OwnedObject;
    using Tests::Support::Blam::Put;
    using Tests::Support::Blam::PutObject;
    namespace ObjeOffset = ::Blam::Layout::Object::Type::Obje::Offset;
    namespace UnitOffset = ::Blam::Layout::Object::Type::Unit::Offset;
    namespace PlayerOffset = ::Blam::Layout::Player::Type::Offset;
    using Contract::Bridge::Type::BridgeTick;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::Pipeline::FillSyntheticTags;
    using Tests::Support::Pipeline::MakeBiped;
    using Tests::Support::Pipeline::WithBipedAndPlayer;

    using Bridge::Codec::Pipeline::System::k_PipelineStageNames;
    using Tests::Support::Pipeline::k_BipedHandle;
    using Tests::Support::Pipeline::k_PlayerHandle;
    using Tests::Support::Pipeline::k_RespawnTicks;

    // Services that write "Cleanup completed." when their layer is cleaned up, with the layer that owns them.
    const std::array<std::string_view, 26> k_CleanupTags{
        "[ClassifierService]", "[ObjectGraphService]",
        "[VitalityDetailsService]", "[ZoneDetailsService]", "[TeleporterDetailsService]", "[LiftDetailsService]",
        "[BarrierDetailsService]", "[ExplosiveDetailsService]", "[ProjectileDetailsService]", "[WeaponDetailsService]", "[EntitiesService]", "[PlayersService]", "[ObjectiveDetailsService]", "[CollidableService]", "[AimService]",
        "[SelfService]", "[AffordanceService]", "[RaycastService]",
        "[ObjectTableService]", "[PlayerTableService]", "[InteractionService]",
        "[MapLoaderService]", "[MapBuilderService]",
        "[WorldBuilder]", "[DefinitionsBuilder]", "[VitalityBuilder]",
    };

    // The seven layers of the pure chain over one Log and a tag index written by hand (D197).
    class PipelineTests : public ::testing::Test, protected Chain
    {
    protected:
        auto SetUp() -> void override
        {
            FillSyntheticTags(m_Map);
        }

        // Runs the pipeline as Mod::Thread::System does (D205) and returns the Tick it assembled.
        auto RunTick(const Snapshot& snapshot, std::uint64_t generation) -> std::shared_ptr<const Tick>
        {
            m_Export.m_PipelineService.Run(snapshot, generation, {});
            return m_Export.m_TickStore.Acquire();
        }

        auto Flatten(const Tick& tick) const -> std::unique_ptr<BridgeTick>
        {
            auto flat{ std::make_unique<BridgeTick>() };
            m_Export.m_FlatService.Flatten(tick, *flat);
            return flat;
        }

        // Cleans up the six layers in the order Mod registers them (D200).
        auto CleanupAll() -> void
        {
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
    };
}

TEST_F(PipelineTests, PipelineBuildsWithoutTheDll)
{
    // Nothing ran yet: no layer published anything and nothing was logged.
    EXPECT_EQ(m_Export.m_TickStore.Acquire(), nullptr);
    EXPECT_EQ(m_Tables.m_ObjectStore.Acquire(), nullptr);
    EXPECT_EQ(m_Environment.m_ClassifierStore.Acquire(), nullptr);
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
    const auto entities{ std::span<const Contract::Bridge::Type::BridgeEntity>{ flat->Entities }.first(flat->Header.EntityCount) };
    EXPECT_TRUE(std::ranges::any_of(entities, [](const auto& entity) { return entity.Handle == k_BipedHandle; }));
}

TEST_F(PipelineTests, RespawnTicksReachTheBridgePlayerWithBody)
{
    const auto tick{ this->RunTick(WithBipedAndPlayer(), 1) };
    ASSERT_NE(tick, nullptr);

    const auto flat{ this->Flatten(*tick) };
    ASSERT_EQ(flat->Header.PlayerCount, 1u);
    EXPECT_EQ(flat->Players[0].HasBody, 1);
    EXPECT_EQ(flat->Players[0].RespawnTicks, static_cast<std::uint32_t>(k_RespawnTicks));
}

TEST_F(PipelineTests, RespawnTicksReachTheBridgePlayerWithoutBody)
{
    Snapshot snapshot{ WithBipedAndPlayer() };
    auto& players{ snapshot.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].Bytes };
    Put(players, (2 * ::Blam::Layout::Player::Type::Size::k_Base) + PlayerOffset::k_AliveBipedHandle, k_NoHandle);

    const auto tick{ this->RunTick(snapshot, 1) };
    ASSERT_NE(tick, nullptr);

    const auto flat{ this->Flatten(*tick) };
    ASSERT_EQ(flat->Header.PlayerCount, 1u);
    EXPECT_EQ(flat->Players[0].HasBody, 0);
    EXPECT_EQ(flat->Players[0].RespawnTicks, static_cast<std::uint32_t>(k_RespawnTicks));
}

TEST_F(PipelineTests, PlayerTraitsReachThePlayerAndTheBridgePlayerWithAndWithoutBody)
{
    const auto expectBaseTraits = [](const auto& traits)
    {
        EXPECT_FLOAT_EQ(traits.MovementSpeed, 1.1f);
        EXPECT_FLOAT_EQ(traits.Gravity, 0.75f);
        EXPECT_FLOAT_EQ(traits.JumpHeight, 1.0f);
        EXPECT_FLOAT_EQ(traits.RadarRange, 25.0f);
        EXPECT_EQ(static_cast<int>(traits.InfiniteAmmo), 2);
        EXPECT_EQ(static_cast<int>(traits.RadarMode), 3);
        EXPECT_EQ(traits.FragGrenades, 2);
        EXPECT_EQ(traits.PlasmaGrenades, 2);
        EXPECT_EQ(traits.PrimaryWeapon, 0);
        EXPECT_EQ(traits.SecondaryWeapon, 1);
    };

    const auto withBody{ this->RunTick(WithBipedAndPlayer(), 1) };
    ASSERT_NE(withBody, nullptr);
    ASSERT_NE(withBody->Players, nullptr);
    ASSERT_EQ(withBody->Players->Rows.size(), 1u);
    EXPECT_TRUE(withBody->Players->Rows[0].IsAlive());
    expectBaseTraits(withBody->Players->Rows[0].Traits);
    const auto flatWith{ this->Flatten(*withBody) };
    ASSERT_EQ(flatWith->Header.PlayerCount, 1u);
    EXPECT_EQ(flatWith->Players[0].HasBody, 1);
    expectBaseTraits(flatWith->Players[0].Traits);

    Snapshot snapshot{ WithBipedAndPlayer() };
    auto& players{ snapshot.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].Bytes };
    Put(players, (2 * ::Blam::Layout::Player::Type::Size::k_Base) + PlayerOffset::k_AliveBipedHandle, k_NoHandle);
    const auto withoutBody{ this->RunTick(snapshot, 1) };
    ASSERT_NE(withoutBody, nullptr);
    ASSERT_NE(withoutBody->Players, nullptr);
    ASSERT_EQ(withoutBody->Players->Rows.size(), 1u);
    EXPECT_FALSE(withoutBody->Players->Rows[0].IsAlive());
    expectBaseTraits(withoutBody->Players->Rows[0].Traits);
    const auto flatWithout{ this->Flatten(*withoutBody) };
    ASSERT_EQ(flatWithout->Header.PlayerCount, 1u);
    EXPECT_EQ(flatWithout->Players[0].HasBody, 0);
    expectBaseTraits(flatWithout->Players[0].Traits);
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

// Environment owns the classification and the object graph since M8 C.1 (D241): its Cleanup() empties them with the rest.
class EnvironmentLayerTests : public ::testing::Test, protected Chain
{
protected:
    auto SetUp() -> void override
    {
        FillSyntheticTags(m_Map);
    }
};

TEST_F(EnvironmentLayerTests, CleanupEmptiesTheClassificationAndTheGraph)
{
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {});
    ASSERT_NE(m_Environment.m_ClassifierStore.Acquire(), nullptr);
    ASSERT_NE(m_Environment.m_ObjectGraphStore.Acquire(), nullptr);
    ASSERT_NE(m_Environment.m_EntitiesStore.Acquire(), nullptr);

    m_Environment.Cleanup();

    EXPECT_EQ(m_Environment.m_ClassifierStore.Acquire(), nullptr);
    EXPECT_EQ(m_Environment.m_ObjectGraphStore.Acquire(), nullptr);
    EXPECT_EQ(m_Environment.m_EntitiesStore.Acquire(), nullptr);
}

TEST_F(PipelineTests, EndStageOncePerStage)
{
    std::size_t calls{ 0 };
    m_Export.m_PipelineService.Run(Snapshot{}, 1, [&calls] { ++calls; });

    EXPECT_EQ(k_PipelineStageNames.size(), 24u);
    EXPECT_EQ(calls, k_PipelineStageNames.size());
}

TEST_F(PipelineTests, RunStopsAfterTheFirstStageWhenThePublishCheckSaysNo)
{
    std::size_t asked{ 0 };
    std::size_t stages{ 0 };

    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, [&stages] { ++stages; }, [&asked] { ++asked; return false; });

    // The check comes after each stage: the stage that was running finishes, the others do not run and no Tick is assembled (F1 C.7).
    EXPECT_EQ(asked, 1u);
    EXPECT_EQ(stages, 1u);
    EXPECT_EQ(m_Export.m_TickStore.Acquire(), nullptr);
}

TEST_F(PipelineTests, RunStopsBetweenStagesWhenTheTeardownStartsInTheMiddle)
{
    std::size_t stages{ 0 };
    const auto first{ RunTick(WithBipedAndPlayer(), 1) };
    ASSERT_NE(first, nullptr);

    // The check says yes for ten stages and then no.
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 2, [&stages] { ++stages; }, [&stages] { return stages < 10; });

    EXPECT_EQ(stages, 10u);
    EXPECT_EQ(m_Export.m_TickStore.Acquire(), first);
}

TEST_F(PipelineTests, EveryStageAsksTheCheck)
{
    std::size_t asked{ 0 };

    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {}, [&asked] { ++asked; return true; });

    // The one after the assembly asks too, with nothing left to stop.
    EXPECT_EQ(asked, k_PipelineStageNames.size());
    EXPECT_NE(m_Export.m_TickStore.Acquire(), nullptr);
}

TEST_F(PipelineTests, TickKeepsTheLastOneWhenThePublishCheckSaysNo)
{
    const auto first{ RunTick(WithBipedAndPlayer(), 1) };
    ASSERT_NE(first, nullptr);

    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 2, {}, [] { return false; });

    EXPECT_EQ(m_Export.m_TickStore.Acquire(), first);
}

TEST_F(PipelineTests, TickIsAssembledWhenThePublishCheckSaysYes)
{
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {}, [] { return true; });

    EXPECT_NE(m_Export.m_TickStore.Acquire(), nullptr);
}

// The links of the objects (F1 C.5 d): a handle with the salt of the live object of its slot matches, another salt is old, an empty slot is free.
class HandleAuditTests : public PipelineTests
{
protected:
    // return: A copy of the biped of WithBipedAndPlayer under another handle, with the three links written.
    static auto MakeLinked(std::uint32_t handle, std::uint32_t parent, std::uint32_t child, std::uint32_t sibling) -> OwnedObject
    {
        OwnedObject object{ MakeBiped() };
        object.Handle = handle;
        Put(object.Bytes, ObjeOffset::k_ParentHandle, parent);
        Put(object.Bytes, ObjeOffset::k_ChildHandle, child);
        Put(object.Bytes, ObjeOffset::k_NextSiblingHandle, sibling);
        return object;
    }

    // return: The snapshot with the biped (slot 3) and two linked objects (slots 4 and 5).
    static auto WithLinks() -> Snapshot
    {
        Snapshot snapshot{ WithBipedAndPlayer() };
        Put(snapshot.RawObjects.Edit(k_BipedHandle), ObjeOffset::k_ChildHandle, k_NoHandle);
        Put(snapshot.RawObjects.Edit(k_BipedHandle), ObjeOffset::k_NextSiblingHandle, k_NoHandle);

        // Parent matches the biped, child has the salt of a deleted object of slot 3, sibling points to the empty slot 9.
        const std::uint32_t second{ MakeHandle(0xEC02, 4) };
        PutObject(snapshot, MakeLinked(second, k_BipedHandle, MakeHandle(0xAAAA, 3), MakeHandle(0xEC03, 9)));

        // Parent with salt 0 over the live slot 4.
        const std::uint32_t third{ MakeHandle(0xEC03, 5) };
        PutObject(snapshot, MakeLinked(third, MakeHandle(0, 4), k_NoHandle, k_NoHandle));
        return snapshot;
    }

    auto ZeroSaltWarnings() const -> std::size_t
    {
        return static_cast<std::size_t>(std::ranges::count_if(m_Sink.m_Messages, [](const auto& message) {
            return message.Level == LogLevel::Warning && message.Message.contains("salt 0");
        }));
    }
};

TEST_F(HandleAuditTests, CountsMatchingStaleFreeAndZeroSaltLinks)
{
    m_Export.m_PipelineService.Run(WithLinks(), 1, {});

    const auto& audit{ m_Tables.m_ObjectService.GetHandleAudit() };
    // The player of the fixture holds the biped as its alive and current biped: two more links that match.
    EXPECT_EQ(audit.Checked, 6u);
    EXPECT_EQ(audit.Matching, 3u);
    EXPECT_EQ(audit.Stale, 2u);
    EXPECT_EQ(audit.Free, 1u);
    EXPECT_EQ(audit.ZeroSalt, 1u);
}

TEST_F(HandleAuditTests, OnlyTheLinksThatExistAreCounted)
{
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {});

    // The biped has no parent, child, sibling or weapon (NONE or 0), and the player holds it twice.
    const auto& audit{ m_Tables.m_ObjectService.GetHandleAudit() };
    EXPECT_EQ(audit.Checked, 2u);
    EXPECT_EQ(audit.Matching, 2u);
    EXPECT_EQ(ZeroSaltWarnings(), 0u);
}

TEST_F(HandleAuditTests, DeadBipedOfThePlayerAndWeaponOfTheBipedWithOldSaltAreStale)
{
    Snapshot snapshot{ WithBipedAndPlayer() };
    auto& players{ snapshot.Regions[static_cast<std::size_t>(Blam::Layout::Snapshot::Type::RegionId::PlayerArray)].Bytes };
    Put(players, (2 * Blam::Layout::Player::Type::Size::k_Base) + PlayerOffset::k_DeadBipedHandle, MakeHandle(0x1111, 3));
    Put(snapshot.RawObjects.Edit(k_BipedHandle), UnitOffset::k_PrimaryWeaponHandle, MakeHandle(0x2222, 6));

    m_Export.m_PipelineService.Run(snapshot, 1, {});

    const auto& audit{ m_Tables.m_ObjectService.GetHandleAudit() };
    EXPECT_EQ(audit.Checked, 4u);
    EXPECT_EQ(audit.Matching, 2u);
    EXPECT_EQ(audit.Stale, 1u);
    EXPECT_EQ(audit.Free, 1u);
    EXPECT_EQ(audit.ZeroSalt, 0u);
    EXPECT_EQ(ZeroSaltWarnings(), 0u);
}

TEST_F(HandleAuditTests, StaleHandlesAreNotLogged)
{
    Snapshot snapshot{ WithLinks() };
    Put(snapshot.RawObjects.Edit(MakeHandle(0xEC03, 5)), ObjeOffset::k_ParentHandle, MakeHandle(0xBBBB, 4));

    m_Export.m_PipelineService.Run(snapshot, 1, {});

    EXPECT_EQ(m_Tables.m_ObjectService.GetHandleAudit().ZeroSalt, 0u);
    EXPECT_GT(m_Tables.m_ObjectService.GetHandleAudit().Stale, 0u);
    EXPECT_EQ(ZeroSaltWarnings(), 0u);
}

TEST_F(HandleAuditTests, ZeroSaltWarningComesOncePerMapLoad)
{
    m_Export.m_PipelineService.Run(WithLinks(), 1, {});
    m_Export.m_PipelineService.Run(WithLinks(), 2, {});
    EXPECT_EQ(ZeroSaltWarnings(), 1u);

    // A new map load runs under another teardown epoch: the next salt 0 is reported again, with no Cleanup needed.
    Snapshot nextLoad{ WithLinks() };
    nextLoad.TeardownEpoch = 1;
    m_Export.m_PipelineService.Run(nextLoad, 3, {});
    m_Export.m_PipelineService.Run(nextLoad, 4, {});
    EXPECT_EQ(ZeroSaltWarnings(), 2u);
}

TEST_F(HandleAuditTests, WeaponSlotsOfAVehicleAreAudited)
{
    Snapshot snapshot{ WithBipedAndPlayer() };
    const std::uint32_t vehicle{ MakeHandle(0xEC09, 8) };
    OwnedObject object{ MakeLinked(vehicle, k_NoHandle, k_NoHandle, k_NoHandle) };
    object.KindId = static_cast<std::uint8_t>(::Blam::MapModel::Definitions::Type::Object::ObjectKind::Vehicle);
    Put(object.Bytes, UnitOffset::k_PrimaryWeaponHandle, vehicle);
    PutObject(snapshot, object);

    m_Export.m_PipelineService.Run(snapshot, 1, {});

    const auto& audit{ m_Tables.m_ObjectService.GetHandleAudit() };
    EXPECT_EQ(audit.Checked, 3u);
    EXPECT_EQ(audit.Matching, 3u);
}