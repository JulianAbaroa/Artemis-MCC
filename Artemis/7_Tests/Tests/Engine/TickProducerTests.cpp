#include <gtest/gtest.h>

import Common.Log.System;
import Common.Log.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Memory.System;
import Engine.Capture.State;
import Engine.Capture.Type;
import Engine.Capture.System;
import Services.Settings.State;
import Services.Telemetry.State;
import Bridge.Codec.Pipeline.Type;
import Bridge.Transport.State;
import Bridge.Transport.System;
import Bridge.Transport.Type;
import Contract.Bridge.Type;
import Mod.Port.System;
import Tests.Support.EngineMemory;
import Tests.Support.FakeBridge;
import Tests.Support.Log;
import std;

// TickProducerAdapter (D305, RV.1): the rules of the game thread that copies a tick and queues it for the AI thread. The real CaptureService
// over fake game memory, the real BridgeService over a store with a fake region, the real queue and the real LifecycleStore: no thread, no
// game. A tick that is lost waits k_ReadTimeout (100 ms) for room in the queue, so the tests that lose ticks cost that each.

namespace
{
    using Common::Log::System::Log;
    using Engine::Base::Lifecycle::State::LifecycleStore;
    using Engine::Base::Lifecycle::Type::Status;
    using Engine::Base::Memory::System::MemoryReaderService;
    using Engine::Capture::State::CaptureStore;
    using Engine::Capture::System::CaptureService;
    using Engine::Capture::Type::ProducedTick;
    using Engine::Capture::Type::k_TickQueueDepth;
    using Services::Settings::State::SettingsStore;
    using Services::Telemetry::State::TelemetryStore;
    using Bridge::Codec::Pipeline::Type::CapturedTick;
    using Bridge::Codec::Pipeline::Type::TickQueue;
    using Bridge::Transport::State::BridgeStore;
    using Bridge::Transport::System::BridgeService;
    using Bridge::Transport::Type::BridgeCounters;
    using Contract::Bridge::Type::BridgeRegion;
    using Mod::Port::System::TickProducerAdapter;
    using Tests::Support::EngineMemory::FakeEngineMemory;
    using Tests::Support::FakeBridge::FakeBridgeEngine;
    using Tests::Support::Log::CollectingSink;

    using std::chrono::milliseconds;

    class TickProducerTests : public testing::Test
    {
    protected:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        LifecycleStore m_Lifecycle{};
        MemoryReaderService m_Reader{};
        CaptureStore m_Store{};
        TelemetryStore m_Telemetry{};
        CaptureService m_Capture{ m_Log, m_Lifecycle, m_Reader, m_Store, m_Telemetry };
        FakeEngineMemory m_Memory{};
        SettingsStore m_Settings{};
        FakeBridgeEngine m_Engine{};
        BridgeStore m_BridgeStore{};
        BridgeService m_Bridge{ m_Log, m_Settings, m_Engine, m_BridgeStore };
        TickQueue m_Queue{ k_TickQueueDepth };
        TickProducerAdapter m_Producer{ m_Lifecycle, m_Capture, m_Bridge, m_Queue };
        std::unique_ptr<BridgeRegion> m_Region{ std::make_unique<BridgeRegion>() };

        // A connected client that asks for a decision every 4 ticks and the queue open, like the AI thread that consumes. The AI thread has
        // not loaded the map yet: a test calls Load.
        auto SetUp() -> void override
        {
            m_Memory.Connect(m_Store);
            m_BridgeStore.SetRegion(m_Region.get(), nullptr, nullptr, nullptr, 0, {});

            auto control{ m_BridgeStore.GetControl() };
            control.IsClientConnected = true;
            control.DecisionPeriod = 4;
            m_BridgeStore.SetControl(control);

            m_Queue.SetOpen(true);
        }

        auto TearDown() -> void override
        {
            m_BridgeStore.ClearRegion();
        }

        // The AI thread loaded the resources of the current teardown epoch.
        auto Load() -> void
        {
            m_Lifecycle.MarkLoaded(m_Lifecycle.GetTeardownEpoch());
        }

        // A teardown and the engine running again.
        auto Teardown() -> void
        {
            m_Lifecycle.SetStatus(Status::TearingDown);
            m_Lifecycle.SetStatus(Status::Running);
        }

        // The client asks for the pipeline only on decision ticks.
        auto SetSparse() -> void
        {
            auto control{ m_BridgeStore.GetControl() };
            control.IsSparseRequested = true;
            m_BridgeStore.SetControl(control);
        }

        // One tick of the game at the action tick: the engine applied it, then the hook asks for the copy.
        auto Produce(std::uint64_t actionTick) -> ProducedTick
        {
            m_BridgeStore.SetActionTick(actionTick);
            return m_Producer.Produce();
        }

        // Fills the queue with ticks that are not the producer's, so the next push waits for room.
        auto Fill() -> void
        {
            while (m_Queue.Size() < k_TickQueueDepth) ASSERT_TRUE(m_Queue.Push(CapturedTick{}, milliseconds{ 0 }).IsQueued);
        }

        // return: What was in the queue, in order.
        auto Drain() -> std::vector<CapturedTick>
        {
            std::vector<CapturedTick> ticks{};
            while (std::optional<CapturedTick> tick{ m_Queue.WaitPop(milliseconds{ 0 }) }) ticks.push_back(std::move(*tick));
            return ticks;
        }

        // return: The only tick in the queue, taken out of it.
        auto TakeOne() -> CapturedTick
        {
            std::vector<CapturedTick> ticks{ this->Drain() };
            EXPECT_EQ(ticks.size(), 1u);
            return ticks.empty() ? CapturedTick{} : std::move(ticks.back());
        }

        auto ClockSkips() const -> std::uint64_t
        {
            BridgeCounters counters{};
            m_BridgeStore.CopyCounters(counters);
            return counters.ClockSkips;
        }

        auto HasSkipWarning() const -> bool
        {
            return std::ranges::any_of(m_Sink.m_Messages, [](const auto& message) { return message.Message.contains("Clock skip"); });
        }
    };
}

TEST_F(TickProducerTests, NothingIsQueuedBeforeTheLoadOfThisEpoch)
{
    // The queue is open and the clock runs, but the AI thread has not loaded the resources: the tick goes by.
    const ProducedTick early{ this->Produce(1) };
    EXPECT_EQ(early.CaptureNs, 0u);
    EXPECT_FALSE(early.IsLost);
    EXPECT_EQ(m_Queue.Size(), 0u);

    this->Load();
    this->Produce(2);
    EXPECT_EQ(m_Queue.Size(), 1u);

    // A teardown raises the epoch: the load of the old map no longer counts, so nothing is queued for the next one until it loads.
    this->Teardown();
    const ProducedTick between{ this->Produce(3) };
    EXPECT_EQ(between.CaptureNs, 0u);
    EXPECT_EQ(m_Queue.Size(), 1u);

    // A tick that goes by while the engine is tearing down is not copied either.
    m_Lifecycle.SetStatus(Status::TearingDown);
    this->Load();
    this->Produce(4);
    EXPECT_EQ(m_Queue.Size(), 1u);

    m_Lifecycle.SetStatus(Status::Running);
    this->Produce(5);
    const std::vector<CapturedTick> ticks{ this->Drain() };
    ASSERT_EQ(ticks.size(), 2u);
    EXPECT_EQ(ticks[0].Epoch, 0u);
    EXPECT_EQ(ticks[1].Epoch, 2u);
}

TEST_F(TickProducerTests, FirstTickAfterALoadIsFullEvenInSparse)
{
    this->Load();
    this->SetSparse();

    this->Produce(1);
    const CapturedTick first{ this->TakeOne() };
    EXPECT_TRUE(first.Clock.IsSparse);
    EXPECT_FALSE(first.IsLight);
    EXPECT_TRUE(first.IsFirstAfterLoad);

    // Then the ticks that are not decisions are light, and a decision is full.
    this->Produce(2);
    const CapturedTick light{ this->TakeOne() };
    EXPECT_TRUE(light.IsLight);
    EXPECT_FALSE(light.IsFirstAfterLoad);

    this->Produce(4);
    const CapturedTick decision{ this->TakeOne() };
    EXPECT_TRUE(decision.Clock.IsDecision);
    EXPECT_FALSE(decision.IsLight);

    this->Produce(5);
    EXPECT_TRUE(this->TakeOne().IsLight);

    // The next load starts again with a full tick.
    this->Teardown();
    this->Load();
    this->Produce(6);
    const CapturedTick reloaded{ this->TakeOne() };
    EXPECT_FALSE(reloaded.IsLight);
    EXPECT_TRUE(reloaded.IsFirstAfterLoad);
}

TEST_F(TickProducerTests, ALostFullTickKeepsTheNextOneFull)
{
    this->Load();
    this->SetSparse();

    // The first tick after the load finds no room: it is lost.
    this->Fill();
    const ProducedTick lost{ this->Produce(1) };
    EXPECT_TRUE(lost.IsLost);
    EXPECT_EQ(m_Queue.Size(), k_TickQueueDepth);
    this->Drain();

    // The next one is still the first after the load, so it is full, and it carries the loss. The numbering has the hole of the lost tick.
    this->Produce(2);
    const CapturedTick next{ this->TakeOne() };
    EXPECT_FALSE(next.IsLight);
    EXPECT_TRUE(next.IsFirstAfterLoad);
    EXPECT_EQ(next.DroppedBefore, 1u);
    EXPECT_EQ(next.Generation, 2u);

    // Only a tick that was queued moves the state forward: this one is light.
    this->Produce(3);
    const CapturedTick light{ this->TakeOne() };
    EXPECT_TRUE(light.IsLight);
    EXPECT_FALSE(light.IsFirstAfterLoad);
    EXPECT_EQ(light.DroppedBefore, 0u);
}

TEST_F(TickProducerTests, ALostDecisionIsNotMarkedPushed)
{
    this->Load();
    this->Fill();

    const ProducedTick lost{ this->Produce(4) };
    EXPECT_TRUE(lost.IsLost);
    EXPECT_EQ(m_BridgeStore.GetPushedDecision(), 0u);
}

TEST_F(TickProducerTests, AQueuedDecisionIsMarkedPushed)
{
    this->Load();

    // "Pushed" means queued for the AI thread: a tick that is not a decision marks nothing.
    this->Produce(3);
    EXPECT_EQ(m_BridgeStore.GetPushedDecision(), 0u);

    this->Produce(4);
    EXPECT_EQ(m_BridgeStore.GetPushedDecision(), 4u);

    this->Produce(5);
    EXPECT_EQ(m_BridgeStore.GetPushedDecision(), 4u);

    const std::vector<CapturedTick> ticks{ this->Drain() };
    ASSERT_EQ(ticks.size(), 3u);
    EXPECT_FALSE(ticks[0].Clock.IsDecision);
    EXPECT_TRUE(ticks[1].Clock.IsDecision);
}

TEST_F(TickProducerTests, AClosedQueueIsNotALostTick)
{
    this->Load();

    // Closed before the copy: the tick goes by, nothing is lost and nothing is counted.
    m_Queue.SetOpen(false);
    const ProducedTick shut{ this->Produce(1) };
    EXPECT_FALSE(shut.IsLost);
    EXPECT_EQ(shut.CaptureNs, 0u);
    EXPECT_EQ(m_Queue.Size(), 0u);

    m_Queue.SetOpen(true);
    this->Produce(2);
    const CapturedTick first{ this->TakeOne() };
    EXPECT_EQ(first.Generation, 1u);
    EXPECT_EQ(first.DroppedBefore, 0u);

    // Closed while the tick is copied: the clock was read, the copy was made, the push finds the queue shut. It is not lost either, and
    // the number it took goes back so the AI thread sees no hole.
    m_Engine.m_OnGetEngineStarts = [this] { m_Queue.SetOpen(false); };
    const ProducedTick closing{ this->Produce(3) };
    EXPECT_FALSE(closing.IsLost);
    EXPECT_EQ(m_Queue.Size(), 0u);

    m_Engine.m_OnGetEngineStarts = nullptr;
    m_Queue.SetOpen(true);
    this->Produce(4);
    const CapturedTick second{ this->TakeOne() };
    EXPECT_EQ(second.Generation, 2u);
    EXPECT_EQ(second.DroppedBefore, 0u);
}

TEST_F(TickProducerTests, AnEpochChangeResetsGenerationLostAndTheBridgeClock)
{
    this->Load();
    this->Produce(1);
    this->Produce(2);
    EXPECT_EQ(this->Drain().back().Generation, 2u);

    // The clock of the new map starts anywhere: the jump is not a skip, because the clock of the old one is forgotten.
    this->Teardown();
    this->Load();
    this->Produce(50);
    const CapturedTick restarted{ this->TakeOne() };
    EXPECT_EQ(restarted.Generation, 1u);
    EXPECT_EQ(restarted.Epoch, 1u);
    EXPECT_TRUE(restarted.IsFirstAfterLoad);
    EXPECT_EQ(this->ClockSkips(), 0u);

    // A tick that was lost does not carry over to the next map.
    this->Fill();
    EXPECT_TRUE(this->Produce(51).IsLost);
    this->Drain();

    this->Teardown();
    this->Load();
    this->Produce(7);
    const CapturedTick again{ this->TakeOne() };
    EXPECT_EQ(again.Generation, 1u);
    EXPECT_EQ(again.DroppedBefore, 0u);
    EXPECT_TRUE(again.IsFirstAfterLoad);
    EXPECT_FALSE(again.IsLight);
    EXPECT_EQ(this->ClockSkips(), 0u);
}

TEST_F(TickProducerTests, DroppedBeforeCarriesTheLostCount)
{
    this->Load();
    this->Fill();
    EXPECT_TRUE(this->Produce(1).IsLost);
    EXPECT_TRUE(this->Produce(2).IsLost);
    this->Drain();

    this->Produce(3);
    const CapturedTick next{ this->TakeOne() };
    EXPECT_EQ(next.DroppedBefore, 2u);
    EXPECT_EQ(next.Generation, 3u);

    // A lost tick is not a skip of the clock, and the count starts over after the one that carries it.
    this->Produce(4);
    const CapturedTick after{ this->TakeOne() };
    EXPECT_EQ(after.DroppedBefore, 0u);
    EXPECT_EQ(this->ClockSkips(), 0u);
}

TEST_F(TickProducerTests, AClosedQueueDoesNotCountAClockSkip)
{
    this->Load();
    this->Produce(1);
    this->Drain();

    // The queue is shut for a few ticks of the game, then the AI thread consumes again: the clock jumped over ticks nobody read.
    m_Queue.SetOpen(false);
    this->Produce(2);
    this->Produce(3);
    this->Produce(4);
    m_Queue.SetOpen(true);
    this->Produce(5);
    EXPECT_EQ(this->ClockSkips(), 0u);
    EXPECT_FALSE(this->HasSkipWarning());
    EXPECT_EQ(this->TakeOne().Clock.ActionTick, 5u);

    // And the same when the queue shuts while a tick is copied, then stays shut.
    m_Engine.m_OnGetEngineStarts = [this] { m_Queue.SetOpen(false); };
    this->Produce(6);
    m_Engine.m_OnGetEngineStarts = nullptr;
    this->Produce(7);
    this->Produce(8);
    m_Queue.SetOpen(true);
    this->Produce(9);
    EXPECT_EQ(this->ClockSkips(), 0u);
    EXPECT_FALSE(this->HasSkipWarning());

    // A jump the producer did see is still a skip: the check is not turned off.
    this->Produce(12);
    EXPECT_EQ(this->ClockSkips(), 1u);
    EXPECT_TRUE(this->HasSkipWarning());
}