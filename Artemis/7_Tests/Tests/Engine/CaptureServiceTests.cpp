#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Log.System;
import Common.Log.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Memory.System;
import Engine.Capture.State;
import Engine.Capture.System;
import Blam.Layout.Object.Type;
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
    using Tests::Support::Log::CollectingSink;

    using Blam::Layout::Object::Type::Constant::k_DataArrayCapacity;
    using Blam::Layout::Object::Type::Constant::k_EntrySize;
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
        CaptureService m_Service{ m_Log, m_Lifecycle, m_Reader, m_Store };

        // The thread context, the headers of the two data arrays and the object table the capture walks.
        std::vector<std::uintptr_t> m_Context{ std::vector<std::uintptr_t>(0x100) };
        std::vector<std::uint8_t> m_ObjectHeader{ std::vector<std::uint8_t>(0x100) };
        std::vector<std::uint8_t> m_PlayerHeader{ std::vector<std::uint8_t>(0x100) };
        std::vector<std::uint8_t> m_ObjectTable{ std::vector<std::uint8_t>(k_MaxObjects * k_EntrySize) };

        auto SetUp() -> void override
        {
            this->Connect();
        }

        // Points the context at the headers and the store at the context and the table, as the locators do at a map load.
        auto Connect() -> void
        {
            m_Context[k_ThreadContextObjectDataOffset / sizeof(std::uintptr_t)] = reinterpret_cast<std::uintptr_t>(m_ObjectHeader.data());
            m_Context[k_ThreadContextTelemetryDataOffset / sizeof(std::uintptr_t)] = reinterpret_cast<std::uintptr_t>(m_PlayerHeader.data());
            m_Store.SetThreadContext(reinterpret_cast<std::uintptr_t>(m_Context.data()));
            m_Store.SetObjectTableBase(reinterpret_cast<std::uintptr_t>(m_ObjectTable.data()));
        }

        auto SetCapacities(std::int32_t objects, std::int32_t players) -> void
        {
            std::memcpy(m_ObjectHeader.data() + k_DataArrayCapacity, &objects, sizeof(objects));
            std::memcpy(m_PlayerHeader.data() + k_DataArrayCapacity, &players, sizeof(players));
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
    EXPECT_EQ(this->CountOf(LogLevel::Info, { "capacities checked", "objects 1024", "players 16" }), 1u);
}

TEST_F(CaptureServiceTests, ObjectCapacityThatDiffersWarnsWithBothValues)
{
    this->SetCapacities(2048, static_cast<std::int32_t>(k_MaxPlayers));

    m_Service.Capture();

    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 1u);
    EXPECT_EQ(this->CountOf(LogLevel::Warning, { "object table", "2048", "1024" }), 1u);
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
    m_Context[k_ThreadContextObjectDataOffset / sizeof(std::uintptr_t)] = 0;
    m_Service.Capture();
    m_Service.Capture();
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);

    this->Connect();
    m_Context[k_ThreadContextTelemetryDataOffset / sizeof(std::uintptr_t)] = 0;
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