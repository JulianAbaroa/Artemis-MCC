#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Log.System;
import Common.Log.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Memory.System;
import Engine.Capture.State;
import Engine.Capture.System;
import Services.Telemetry.State;
import Services.Telemetry.Type;
import Blam.Layout.Object.Type;
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
    using Engine::Base::Lifecycle::State::LifecycleStore;
    using Engine::Base::Lifecycle::Type::Status;
    using Engine::Base::Memory::System::MemoryReaderService;
    using Engine::Capture::State::CaptureStore;
    using Engine::Capture::System::CaptureService;
    using Services::Telemetry::State::TelemetryStore;
    using Services::Telemetry::Type::PerfSnapshot;
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
            return snapshot->RawObjects.contains((1u << 16) | index);
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
    EXPECT_EQ(snapshot->RawObjects.size(), 2u);
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