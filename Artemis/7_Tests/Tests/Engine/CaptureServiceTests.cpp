#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.State.Type;
import Common.Log.System;
import Common.Log.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Memory.System;
import Engine.Capture.State;
import Engine.Capture.Type;
import Engine.Capture.System;
import Engine.Control.Input.State;
import Engine.Control.Input.Type;
import Services.Telemetry.State;
import Services.Telemetry.Type;
import Blam.Layout.Action.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Snapshot.System;
import Blam.Layout.Snapshot.Type;
import Blam.MapModel.Definitions.Type;
import Tests.Support.EngineMemory;
import Tests.Support.Log;
import std;

// The self-checks of CaptureService (F1 C.5 c): the capacity of the object and player tables of the engine against what Artemis assumes,
// once per map load. The "game memory" is a set of buffers of the test process, which MemoryReaderService reads like any address.

namespace
{
    using Common::Engine::Type::k_MaxObjects;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Log::System::Log;
    using Common::Log::Type::LogLevel;
    using Common::State::Type::BoundedQueue;
    using Engine::Base::Lifecycle::State::LifecycleStore;
    using Engine::Control::Input::State::PlayerActionStore;
    using Engine::Control::Input::Type::PlayerActions;
    using Blam::Layout::Action::Type::Offset::k_AppliedActionsOffset;
    using Blam::Layout::Action::Type::Offset::k_AppliedMaskOffset;
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
    using Blam::Layout::Action::Type::Size::k_PlayerActionSize;
    using Engine::Base::Lifecycle::Type::Status;
    using Engine::Base::Memory::System::MemoryReaderService;
    using Engine::Capture::State::CaptureStore;
    using Engine::Capture::System::CaptureService;
    using Engine::Capture::Type::k_TickQueueDepth;
    using Services::Telemetry::State::TelemetryStore;
    using Services::Telemetry::Type::PerfSnapshot;
    using Blam::Layout::Snapshot::Type::IsLightRegion;
    using Blam::Layout::Snapshot::Type::RegionId;
    using Blam::Layout::Snapshot::Type::RegionSize;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::Layout::Snapshot::Type::k_LightKindId;
    using Blam::Layout::Snapshot::Type::k_RegionCount;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::MapModel::Definitions::Type::Object::ToObjectKind;
    using Blam::Layout::Snapshot::System::MakeLight;
    using Tests::Support::EngineMemory::FakeEngineMemory;
    using Tests::Support::Log::CollectingSink;

    using Blam::Layout::Object::Type::Constant::k_ThreadContextObjectDataOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTelemetryDataOffset;

    class CaptureServiceTests : public testing::Test
    {
    protected:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        LifecycleStore m_Lifecycle{};
        MemoryReaderService m_Reader{};
        CaptureStore m_Store{};
        TelemetryStore m_Telemetry{};
        CaptureService m_Service{ m_Log, m_Lifecycle, m_Reader, m_Store, m_Telemetry };

        // The fake game memory: the thread context, the headers of the two data arrays and the object table the capture walks.
        FakeEngineMemory m_Memory{};

        auto SetUp() -> void override
        {
            this->Connect();
        }

        // Points the context at the headers and the store at the context and the table, as the locators do at a map load.
        auto Connect() -> void
        {
            m_Memory.Connect(m_Store);
        }

        auto SetCapacities(std::int32_t objects, std::int32_t players) -> void
        {
            m_Memory.SetCapacities(objects, players);
        }

        // Sets the first-unused slot (+0x44) and the live count (+0x48) of the object data array.
        auto SetMark(std::int32_t firstUnused, std::int32_t liveCount) -> void
        {
            m_Memory.SetMark(firstUnused, liveCount);
        }

        // Makes the slot of the table a live object with salt 1: its handle is 0x10000 | index.
        auto AddLive(std::uint32_t index) -> void
        {
            m_Memory.AddLive(index);
        }

        // return: Whether the snapshot has the object of that slot.
        static auto Has(const auto& snapshot, std::uint32_t index) -> bool
        {
            return snapshot->RawObjects.Contains((1u << 16) | index);
        }

        // return: Messages of the level whose text has all the parts.
        auto CountOf(LogLevel level, std::initializer_list<std::string_view> parts) const -> std::size_t
        {
            return static_cast<std::size_t>(std::ranges::count_if(m_Sink.m_Messages, [&](const auto& message) {
                return message.Level == level &&
                    std::ranges::all_of(parts, [&](std::string_view part) { return message.Message.contains(part); });
            }));
        }
    };
}

TEST_F(CaptureServiceTests, CapacitiesThatMatchLogNoWarning)
{
    this->SetCapacities(static_cast<std::int32_t>(k_MaxObjects), static_cast<std::int32_t>(k_MaxPlayers));

    m_Service.Capture();

    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
    EXPECT_EQ(this->CountOf(LogLevel::Info, { "capacities checked", "objects 2048", "players 16" }), 1u);
}

TEST_F(CaptureServiceTests, ObjectCapacityThatDiffersWarnsWithBothValues)
{
    this->SetCapacities(4096, static_cast<std::int32_t>(k_MaxPlayers));

    m_Service.Capture();

    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 1u);
    EXPECT_EQ(this->CountOf(LogLevel::Warning, { "object table", "4096", "2048" }), 1u);
}

TEST_F(CaptureServiceTests, PlayerCapacityThatDiffersWarnsWithBothValues)
{
    this->SetCapacities(static_cast<std::int32_t>(k_MaxObjects), 32);

    m_Service.Capture();

    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 1u);
    EXPECT_EQ(this->CountOf(LogLevel::Warning, { "player table", "32", "16" }), 1u);
}

TEST_F(CaptureServiceTests, WarningComesOncePerMapLoadNotPerTick)
{
    this->SetCapacities(512, 8);

    for (int tick{ 0 }; tick < 5; ++tick) m_Service.Capture();
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 2u);

    // The next map load runs under another teardown epoch: it is compared again. Cleanup does not have to reset anything.
    m_Lifecycle.SetStatus(Status::TearingDown);
    m_Service.Cleanup();
    this->Connect();
    for (int tick{ 0 }; tick < 5; ++tick) m_Service.Capture();
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 4u);
}

TEST_F(CaptureServiceTests, ArrayNotAllocatedYetIsCheckedLater)
{
    // Capacities of 0 are an array the engine has not set up (or a failed read): nothing is said and nothing is marked as checked.
    this->SetCapacities(0, 0);
    m_Service.Capture();
    m_Service.Capture();
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
    EXPECT_EQ(this->CountOf(LogLevel::Info, { "capacities checked" }), 0u);

    this->SetCapacities(512, static_cast<std::int32_t>(k_MaxPlayers));
    m_Service.Capture();
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 1u);
}

TEST_F(CaptureServiceTests, NoThreadContextChecksNothing)
{
    this->SetCapacities(512, 8);
    m_Store.SetThreadContext(0);

    m_Service.Capture();

    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
}

TEST_F(CaptureServiceTests, NullHeaderPointerChecksNothingAndIsRetriedOnTheNextTick)
{
    this->SetCapacities(512, 8);
    m_Memory.m_Context[k_ThreadContextObjectDataOffset / sizeof(std::uintptr_t)] = 0;
    m_Service.Capture();
    m_Service.Capture();
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);

    this->Connect();
    m_Memory.m_Context[k_ThreadContextTelemetryDataOffset / sizeof(std::uintptr_t)] = 0;
    m_Service.Capture();
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);

    this->Connect();
    m_Service.Capture();
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 2u);
}

TEST_F(CaptureServiceTests, SnapshotCarriesTheTeardownEpoch)
{
    this->SetCapacities(static_cast<std::int32_t>(k_MaxObjects), static_cast<std::int32_t>(k_MaxPlayers));
    EXPECT_EQ(m_Service.Capture()->TeardownEpoch, 0u);

    m_Lifecycle.SetStatus(Status::TearingDown);
    EXPECT_EQ(m_Service.Capture()->TeardownEpoch, 1u);
}

TEST_F(CaptureServiceTests, ObjectsBelowTheMarkAreCopiedAndNoneAbove)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));
    this->SetMark(10, 2);
    this->AddLive(3);
    this->AddLive(9);
    this->AddLive(10);
    this->AddLive(1500);

    const auto snapshot{ m_Service.Capture() };

    EXPECT_TRUE(Has(snapshot, 3));
    EXPECT_TRUE(Has(snapshot, 9));
    EXPECT_FALSE(Has(snapshot, 10));
    EXPECT_FALSE(Has(snapshot, 1500));
    EXPECT_EQ(snapshot->RawObjects.Size(), 2u);
}

TEST_F(CaptureServiceTests, ObjectAtAnIndexOf1024OrMoreIsCopied)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));
    this->SetMark(1501, 2);
    this->AddLive(5);
    this->AddLive(1024);
    this->AddLive(1500);

    const auto snapshot{ m_Service.Capture() };

    EXPECT_TRUE(Has(snapshot, 5));
    EXPECT_TRUE(Has(snapshot, 1024));
    EXPECT_TRUE(Has(snapshot, 1500));
}

TEST_F(CaptureServiceTests, MarkOfZeroWalksTheCapacity)
{
    this->SetCapacities(100, static_cast<std::int32_t>(k_MaxPlayers));
    this->SetMark(0, 0);
    this->AddLive(5);
    this->AddLive(99);
    this->AddLive(100);

    const auto snapshot{ m_Service.Capture() };

    EXPECT_TRUE(Has(snapshot, 5));
    EXPECT_TRUE(Has(snapshot, 99));
    EXPECT_FALSE(Has(snapshot, 100));
}

TEST_F(CaptureServiceTests, UnreadableArrayWalksEverySlotUpToTheLimit)
{
    m_Memory.m_Context[k_ThreadContextObjectDataOffset / sizeof(std::uintptr_t)] = 0;
    this->AddLive(5);
    this->AddLive(k_MaxObjects - 1);
    this->AddLive(k_MaxObjects);

    const auto snapshot{ m_Service.Capture() };

    EXPECT_TRUE(Has(snapshot, 5));
    EXPECT_TRUE(Has(snapshot, k_MaxObjects - 1));
    EXPECT_FALSE(Has(snapshot, k_MaxObjects));
}

TEST_F(CaptureServiceTests, CapacityLargerThanTheLimitWalksTheLimitAndWarns)
{
    this->SetCapacities(4096, static_cast<std::int32_t>(k_MaxPlayers));
    this->SetMark(4096, 3);
    this->AddLive(7);
    this->AddLive(k_MaxObjects - 1);
    this->AddLive(k_MaxObjects);

    const auto snapshot{ m_Service.Capture() };

    EXPECT_TRUE(Has(snapshot, 7));
    EXPECT_TRUE(Has(snapshot, k_MaxObjects - 1));
    EXPECT_FALSE(Has(snapshot, k_MaxObjects));
    EXPECT_EQ(this->CountOf(LogLevel::Warning, { "object table", "4096", "2048" }), 1u);
}

TEST_F(CaptureServiceTests, TheMarkIsReadOnEveryCapture)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));
    this->AddLive(20);
    this->SetMark(10, 1);
    EXPECT_FALSE(Has(m_Service.Capture(), 20));

    this->SetMark(21, 1);
    EXPECT_TRUE(Has(m_Service.Capture(), 20));
}

TEST_F(CaptureServiceTests, TelemetryKeepsTheHighestMarkAndLiveCountUntilThePhoto)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));
    this->SetMark(300, 120);
    m_Service.Capture();
    this->SetMark(1700, 640);
    m_Service.Capture();
    this->SetMark(50, 10);
    m_Service.Capture();

    PerfSnapshot photo{};
    m_Telemetry.CopyPerf(photo);
    EXPECT_EQ(photo.ObjectSlotsMax, 1700u);
    EXPECT_EQ(photo.ObjectsMax, 640u);

    // The photo restarts the maxima: the next one only sees what came after.
    m_Service.Capture();
    m_Telemetry.CopyPerf(photo);
    EXPECT_EQ(photo.ObjectSlotsMax, 50u);
    EXPECT_EQ(photo.ObjectsMax, 10u);
}

TEST_F(CaptureServiceTests, SnapshotHeldByAReaderIsNotReusedAndKeepsItsValues)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));
    this->SetMark(10, 1);
    this->AddLive(3);
    const auto held{ m_Service.Capture() };
    ASSERT_TRUE(Has(held, 3));
    const std::vector<std::uint8_t> before{ held->RawObjects.Find((1u << 16) | 3u)->Bytes.begin(), held->RawObjects.Find((1u << 16) | 3u)->Bytes.end() };

    // Other data in the memory and 3 more captures: more than the pool would need if the held one could be taken.
    std::ranges::fill(m_Memory.m_Datum, std::uint8_t{ 0x5A });
    this->AddLive(7);
    this->SetMark(10, 2);
    for (int tick{ 0 }; tick < 3; ++tick) EXPECT_NE(m_Service.Capture().get(), held.get());

    EXPECT_TRUE(Has(held, 3));
    EXPECT_FALSE(Has(held, 7));
    EXPECT_TRUE(std::ranges::equal(held->RawObjects.Find((1u << 16) | 3u)->Bytes, before));
}

TEST_F(CaptureServiceTests, ReleasedSnapshotsAreReusedAndOnlyTheNewObjectsRemain)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));
    this->SetMark(10, 3);
    this->AddLive(3);
    this->AddLive(5);
    this->AddLive(8);

    std::set<const void*> addresses{};
    for (int tick{ 0 }; tick < 10; ++tick)
    {
        const auto snapshot{ m_Service.Capture() };
        addresses.insert(snapshot.get());
        EXPECT_EQ(snapshot->RawObjects.Size(), 3u);
    }

    // The published one is never taken: two snapshots take turns.
    EXPECT_EQ(addresses.size(), 2u);

    // Fewer objects than before: nothing of the old tick is left in the reused one.
    this->SetMark(4, 1);
    for (int tick{ 0 }; tick < 4; ++tick)
    {
        const auto snapshot{ m_Service.Capture() };
        EXPECT_EQ(snapshot->RawObjects.Size(), 1u);
        EXPECT_TRUE(Has(snapshot, 3));
        EXPECT_FALSE(Has(snapshot, 5));
        EXPECT_FALSE(Has(snapshot, 8));
    }
}

TEST_F(CaptureServiceTests, ReusedSnapshotDropsTheRegionThatIsNotCapturedAnymore)
{
    Snapshot withTime{};
    withTime.HasThreadContext = true;
    withTime.HasObjectTable = true;
    withTime.Regions[static_cast<std::size_t>(RegionId::TimeGlobals)].OriginalAddress = 0x1000;
    withTime.Regions[static_cast<std::size_t>(RegionId::TimeGlobals)].Bytes.assign(RegionSize(RegionId::TimeGlobals), 0xAB);
    Snapshot without{};
    without.HasThreadContext = true;
    without.HasObjectTable = true;

    for (int round{ 0 }; round < 4; ++round)
    {
        m_Memory.Load(withTime, m_Store);
        {
            const auto snapshot{ m_Service.Capture() };
            ASSERT_TRUE(snapshot->IsCaptured(RegionId::TimeGlobals));
            EXPECT_EQ(snapshot->Read<std::uint8_t>(RegionId::TimeGlobals, 0), 0xABu);
        }

        m_Memory.Load(without, m_Store);
        {
            const auto snapshot{ m_Service.Capture() };
            EXPECT_FALSE(snapshot->IsCaptured(RegionId::TimeGlobals));
            EXPECT_EQ(snapshot->Regions[static_cast<std::size_t>(RegionId::TimeGlobals)].OriginalAddress, 0u);
        }
    }
}

TEST_F(CaptureServiceTests, ReusedSnapshotDropsTheContextRegionsWhenTheContextOrTheMegaloPointerIsMissing)
{
    const std::array<RegionId, 5> contextRegions{ RegionId::TimeGlobals, RegionId::GameEngineGlobals, RegionId::EventGlobals,
        RegionId::GameGlobals, RegionId::MegaloObjects };

    Snapshot full{};
    full.HasThreadContext = true;
    full.HasObjectTable = true;
    for (const RegionId id : contextRegions)
    {
        auto& region{ full.Regions[static_cast<std::size_t>(id)] };
        region.OriginalAddress = 0x1000;
        region.Bytes.assign(RegionSize(id), 0xAB);
    }

    Snapshot noContext{};
    noContext.HasObjectTable = true;

    Snapshot noMegalo{ full };
    noMegalo.Regions[static_cast<std::size_t>(RegionId::MegaloObjects)] = {};

    for (int round{ 0 }; round < 4; ++round)
    {
        m_Memory.Load(full, m_Store);
        {
            const auto snapshot{ m_Service.Capture() };
            for (const RegionId id : contextRegions) ASSERT_TRUE(snapshot->IsCaptured(id));
        }

        m_Memory.Load(noContext, m_Store);
        {
            const auto snapshot{ m_Service.Capture() };
            for (const RegionId id : contextRegions)
            {
                EXPECT_FALSE(snapshot->IsCaptured(id));
                EXPECT_EQ(snapshot->Regions[static_cast<std::size_t>(id)].OriginalAddress, 0u);
            }
        }

        m_Memory.Load(full, m_Store);
        {
            const auto snapshot{ m_Service.Capture() };
            ASSERT_TRUE(snapshot->IsCaptured(RegionId::MegaloObjects));
        }

        m_Memory.Load(noMegalo, m_Store);
        {
            const auto snapshot{ m_Service.Capture() };
            EXPECT_FALSE(snapshot->IsCaptured(RegionId::MegaloObjects));
            EXPECT_EQ(snapshot->Regions[static_cast<std::size_t>(RegionId::MegaloObjects)].OriginalAddress, 0u);
            EXPECT_TRUE(snapshot->IsCaptured(RegionId::TimeGlobals));
        }
    }
}

TEST_F(CaptureServiceTests, ReusedSnapshotGetsTheNewEpochAndCleanupDoesNotBreakTheNextCapture)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));
    for (int tick{ 0 }; tick < 4; ++tick) EXPECT_EQ(m_Service.Capture()->TeardownEpoch, 0u);

    m_Lifecycle.SetStatus(Status::TearingDown);
    m_Service.Cleanup();
    EXPECT_EQ(m_Store.Acquire(), nullptr);
    this->Connect();

    for (int tick{ 0 }; tick < 4; ++tick) EXPECT_EQ(m_Service.Capture()->TeardownEpoch, 1u);
}


// Q101.B: the light capture of the ticks without pipeline.
namespace
{
    // A snapshot with objects of several kinds (the weapons are the ones of kind k_LightKindId) and every region filled with a pattern.
    auto MakeSourceWithWeapons() -> Snapshot
    {
        Snapshot source{};
        source.HasThreadContext = true;
        source.HasObjectTable = true;

        const std::array<std::pair<std::uint32_t, std::uint8_t>, 6> objects{ { { 3, 0 }, { 7, k_LightKindId }, { 20, 3 }, { 21, k_LightKindId },
            { 40, 9 }, { 1500, k_LightKindId } } };
        for (const auto& [index, kind] : objects)
        {
            const auto bytes{ source.RawObjects.Emplace((1u << 16) | index, 0, kind, 0x200) };
            for (std::size_t i{ 0 }; i < bytes->size(); ++i) (*bytes)[i] = static_cast<std::uint8_t>(index + i);
        }

        for (std::size_t id{ 0 }; id < k_RegionCount; ++id)
        {
            source.Regions[id].OriginalAddress = 0x1000;
            source.Regions[id].Bytes.assign(RegionSize(static_cast<RegionId>(id)), static_cast<std::uint8_t>(0x10 + id));
        }

        return source;
    }

    auto ExpectSameSnapshot(const Snapshot& actual, const Snapshot& expected) -> void
    {
        EXPECT_EQ(actual.IsLight, expected.IsLight);
        EXPECT_EQ(actual.HasThreadContext, expected.HasThreadContext);
        EXPECT_EQ(actual.HasObjectTable, expected.HasObjectTable);
        EXPECT_EQ(actual.TeardownEpoch, expected.TeardownEpoch);

        ASSERT_EQ(actual.RawObjects.Size(), expected.RawObjects.Size());
        for (auto a{ actual.RawObjects.begin() }, e{ expected.RawObjects.begin() }; a != actual.RawObjects.end(); ++a, ++e)
        {
            EXPECT_EQ(a->Handle, e->Handle);
            EXPECT_EQ(a->KindId, e->KindId);
            EXPECT_EQ(a->Address, e->Address);
            EXPECT_TRUE(std::ranges::equal(a->Bytes, e->Bytes));
        }

        for (std::size_t id{ 0 }; id < k_RegionCount; ++id)
        {
            EXPECT_EQ(actual.Regions[id].OriginalAddress, expected.Regions[id].OriginalAddress) << "region " << id;
            EXPECT_EQ(actual.Regions[id].Bytes, expected.Regions[id].Bytes) << "region " << id;
        }
    }
}

TEST_F(CaptureServiceTests, TheLightKindIsTheWeaponKindOfTheMapModel)
{
    EXPECT_EQ(ToObjectKind(k_LightKindId), ObjectKind::Weapon);
}

TEST_F(CaptureServiceTests, CaptureLightIsTheOfflineLightSnapshotOfTheFullCapture)
{
    m_Memory.Load(MakeSourceWithWeapons(), m_Store);

    const auto full{ m_Service.Capture() };
    const auto light{ m_Service.CaptureLight() };

    EXPECT_TRUE(light->IsLight);
    EXPECT_FALSE(full->IsLight);
    ExpectSameSnapshot(*light, MakeLight(*full));

    // Only the weapons and the light regions.
    EXPECT_EQ(light->RawObjects.Size(), 3u);
    EXPECT_TRUE(Has(light, 7));
    EXPECT_TRUE(Has(light, 21));
    EXPECT_TRUE(Has(light, 1500));
    EXPECT_FALSE(Has(light, 3));
    for (std::size_t id{ 0 }; id < k_RegionCount; ++id)
    {
        EXPECT_EQ(light->IsCaptured(static_cast<RegionId>(id)), IsLightRegion(static_cast<RegionId>(id))) << "region " << id;
    }
    EXPECT_TRUE(IsLightRegion(RegionId::TimeGlobals));
    EXPECT_TRUE(IsLightRegion(RegionId::GameEngineGlobals));
    EXPECT_TRUE(IsLightRegion(RegionId::MegaloObjects));
}

TEST_F(CaptureServiceTests, CaptureLightDoesNotPublishNorTouchTheTelemetryOfTheObjectTable)
{
    m_Memory.Load(MakeSourceWithWeapons(), m_Store);
    this->SetMark(300, 120);
    const auto full{ m_Service.Capture() };

    this->SetMark(1700, 640);
    const auto light{ m_Service.CaptureLight() };

    EXPECT_EQ(m_Store.Acquire(), full);

    PerfSnapshot photo{};
    m_Telemetry.CopyPerf(photo);
    EXPECT_EQ(photo.ObjectSlotsMax, 300u);
    EXPECT_EQ(photo.ObjectsMax, 120u);
}

TEST_F(CaptureServiceTests, LightAndFullCapturesShareTheSnapshotPoolWithoutLeavingAnythingBehind)
{
    const Snapshot source{ MakeSourceWithWeapons() };
    m_Memory.Load(source, m_Store);

    for (int round{ 0 }; round < 6; ++round)
    {
        {
            const auto light{ m_Service.CaptureLight() };
            EXPECT_EQ(light->RawObjects.Size(), 3u);
            EXPECT_FALSE(light->IsCaptured(RegionId::PlayerArray));
            EXPECT_FALSE(light->IsCaptured(RegionId::Placements));
        }
        {
            const auto full{ m_Service.Capture() };
            EXPECT_FALSE(full->IsLight);
            EXPECT_EQ(full->RawObjects.Size(), 6u);
            EXPECT_TRUE(full->IsCaptured(RegionId::PlayerArray));
            EXPECT_TRUE(full->IsCaptured(RegionId::EventGlobals));
        }
    }
}

TEST_F(CaptureServiceTests, CaptureLightWithoutThreadContextHoldsNothing)
{
    m_Memory.Load(MakeSourceWithWeapons(), m_Store);
    m_Store.Cleanup();

    const auto light{ m_Service.CaptureLight() };

    EXPECT_TRUE(light->IsLight);
    EXPECT_FALSE(light->HasThreadContext);
    EXPECT_TRUE(light->RawObjects.IsEmpty());
    EXPECT_FALSE(light->IsCaptured(RegionId::TimeGlobals));
}

// D305: the game thread fills the pool (Capture) and the AI thread releases the Snapshots. Every queued Snapshot keeps the contents of its
// own capture while the producer goes on, and no Snapshot is in flight twice.
TEST_F(CaptureServiceTests, SnapshotsCapturedOnOneThreadAndReleasedOnAnotherAreNeitherOverwrittenNorHandedOutTwice)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));
    this->AddLive(3);
    this->AddLive(5);
    this->AddLive(8);

    // The mark decides how many of the three objects each capture finds: 4 -> 1, 6 -> 2, 9 -> 3.
    constexpr std::array<std::int32_t, 3> k_Marks{ 4, 6, 9 };

    struct QueuedSnapshot
    {
        std::shared_ptr<const Snapshot> Value{};
        std::size_t ExpectedObjects{};
    };

    BoundedQueue<QueuedSnapshot> queue{ k_TickQueueDepth };
    queue.SetOpen(true);

    std::mutex inFlightMutex{};
    std::set<const void*> inFlight{};
    std::atomic<int> overwritten{ 0 };
    std::atomic<int> duplicated{ 0 };
    constexpr int k_Ticks{ 600 };

    std::jthread consumer{ [&] {
        int taken{ 0 };
        while (taken < k_Ticks)
        {
            std::optional<QueuedSnapshot> item{ queue.WaitPop(std::chrono::milliseconds{ 100 }) };
            if (!item.has_value()) continue;

            // Slow enough that the queue fills and the producer has to reuse the pool while this one is held.
            if (taken % 5 == 0) std::this_thread::sleep_for(std::chrono::microseconds{ 300 });
            if (item->Value->RawObjects.Size() != item->ExpectedObjects) ++overwritten;

            {
                std::lock_guard lock{ inFlightMutex };
                inFlight.erase(item->Value.get());
            }
            item.reset();
            ++taken;
        }
    } };

    for (int tick{ 0 }; tick < k_Ticks; ++tick)
    {
        const std::size_t shape{ static_cast<std::size_t>(tick) % k_Marks.size() };
        this->SetMark(k_Marks[shape], static_cast<std::int32_t>(shape + 1));

        auto snapshot{ m_Service.Capture() };
        {
            std::lock_guard lock{ inFlightMutex };
            if (!inFlight.insert(snapshot.get()).second) ++duplicated;
        }

        EXPECT_TRUE(queue.Push(QueuedSnapshot{ std::move(snapshot), shape + 1 }, std::chrono::milliseconds{ 5000 }).IsQueued);
    }
    consumer.join();

    EXPECT_EQ(overwritten.load(), 0);
    EXPECT_EQ(duplicated.load(), 0);
}


// F5.1 A (D371): the region PlayerActions is not game memory, the owner of the applied actions gives it through a port.
namespace
{
    // The actions of one tick as the hook of the engine publishes them: tick number in the mask and a pattern in each action.
    auto MakeActions(std::uint64_t tick) -> PlayerActions
    {
        PlayerActions actions{};
        actions.Sequence = tick;
        actions.Mask = static_cast<std::uint32_t>(0x8000'0000u | tick);
        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
        {
            for (std::size_t i{ 0 }; i < k_PlayerActionSize; ++i)
            {
                actions.Actions[player].Data[i] = static_cast<std::uint8_t>(tick + (player * 13) + (i * 3));
            }
        }
        return actions;
    }

    auto MaskOf(const Snapshot& snapshot) -> std::uint32_t
    {
        return snapshot.Read<std::uint32_t>(RegionId::PlayerActions, k_AppliedMaskOffset);
    }
}

TEST_F(CaptureServiceTests, PlayerActionsHasTheMaskAndTheSixteenActionsOfThePort)
{
    PlayerActionStore actions{};
    const PlayerActions published{ MakeActions(7) };
    actions.Publish(published);
    m_Service.SetAppliedActionsPort(&actions);

    const auto snapshot{ m_Service.Capture() };

    ASSERT_TRUE(snapshot->IsCaptured(RegionId::PlayerActions));
    EXPECT_EQ(snapshot->Regions[static_cast<std::size_t>(RegionId::PlayerActions)].Bytes.size(), RegionSize(RegionId::PlayerActions));
    EXPECT_EQ(MaskOf(*snapshot), published.Mask);
    for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
    {
        const auto copy{ snapshot->ReadArray<std::uint8_t, k_PlayerActionSize>(RegionId::PlayerActions, k_AppliedActionsOffset + (player * k_PlayerActionSize)) };
        EXPECT_EQ(copy, published.Actions[player].Data) << "player " << player;
    }
}

TEST_F(CaptureServiceTests, PlayerActionsIsEmptyWithoutAPortOrBeforeTheFirstTick)
{
    // No port: empty, and the rest of the capture is untouched.
    EXPECT_FALSE(m_Service.Capture()->IsCaptured(RegionId::PlayerActions));

    PlayerActionStore actions{};
    m_Service.SetAppliedActionsPort(&actions);
    EXPECT_FALSE(m_Service.Capture()->IsCaptured(RegionId::PlayerActions)) << "nothing published yet";

    actions.Publish(MakeActions(1));
    EXPECT_TRUE(m_Service.Capture()->IsCaptured(RegionId::PlayerActions));

    // A reused snapshot never keeps the region of an older capture.
    m_Service.SetAppliedActionsPort(nullptr);
    for (int tick{ 0 }; tick < 10; ++tick) EXPECT_FALSE(m_Service.Capture()->IsCaptured(RegionId::PlayerActions));

    m_Service.SetAppliedActionsPort(&actions);
    actions.Cleanup();
    for (int tick{ 0 }; tick < 10; ++tick) EXPECT_FALSE(m_Service.Capture()->IsCaptured(RegionId::PlayerActions));
    EXPECT_EQ(m_Sink.m_Messages.size(), 0u) << "no log per tick";
}

// What it checks: the region is the latest the port gave when the capture runs. It cannot see the order of the hooks: that the capture
// that closes tick N runs after PlayerActionDetour published tick N and before tick N + 1 is read from ExecuteGameFrameDetour and
// PlayerActionDetour (still to confirm in game: the tick of TimeGlobals against the Sequence).
TEST_F(CaptureServiceTests, PlayerActionsAreTheLatestThePortGaveWhenTheCaptureRuns)
{
    PlayerActionStore actions{};
    m_Service.SetAppliedActionsPort(&actions);

    for (std::uint64_t tick{ 1 }; tick <= 12; ++tick)
    {
        actions.Publish(MakeActions(tick));
        const auto closed{ m_Service.Capture() };
        EXPECT_EQ(MaskOf(*closed), static_cast<std::uint32_t>(0x8000'0000u | tick)) << "tick " << tick;
    }
}

// A tick that publishes nothing (the null update at the end of a match) leaves the Sequence as it was: its region would be a stale repeat.
TEST_F(CaptureServiceTests, PlayerActionsAreEmptyWhenTheSequenceDidNotAdvance)
{
    PlayerActionStore actions{};
    m_Service.SetAppliedActionsPort(&actions);

    actions.Publish(MakeActions(5));
    const auto first{ m_Service.Capture() };
    ASSERT_TRUE(first->IsCaptured(RegionId::PlayerActions));
    EXPECT_EQ(MaskOf(*first), static_cast<std::uint32_t>(0x8000'0000u | 5));

    for (int tick{ 0 }; tick < 3; ++tick) EXPECT_FALSE(m_Service.Capture()->IsCaptured(RegionId::PlayerActions));

    actions.Publish(MakeActions(6));
    const auto next{ m_Service.Capture() };
    ASSERT_TRUE(next->IsCaptured(RegionId::PlayerActions));
    EXPECT_EQ(MaskOf(*next), static_cast<std::uint32_t>(0x8000'0000u | 6));
}

TEST_F(CaptureServiceTests, ALightCaptureHasNoPlayerActions)
{
    PlayerActionStore actions{};
    actions.Publish(MakeActions(3));
    m_Service.SetAppliedActionsPort(&actions);

    EXPECT_TRUE(m_Service.Capture()->IsCaptured(RegionId::PlayerActions));
    for (int tick{ 0 }; tick < 10; ++tick) EXPECT_FALSE(m_Service.CaptureLight()->IsCaptured(RegionId::PlayerActions));
}
