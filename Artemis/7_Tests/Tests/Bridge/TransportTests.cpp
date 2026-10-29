#include <gtest/gtest.h>
#include <windows.h>

import Common.Log.System;
import Common.Log.Type;
import Control.Action.Type;
import Services.Settings.State;
import Bridge.Codec.Flat.Type;
import Bridge.Transport.Type;
import Bridge.Transport.State;
import Bridge.Transport.System;
import Tests.Support.Log;
import std;

// Bridge.Transport (M6b, D215): the real BridgeService over its shared memory and events, with a fake engine behind the BridgeEngine port.
// The test acts as the Python client: it writes the control block and the actions of the region and calls Update, Apply, BeginTick and
// EndTick the way the threads of the game do. The tests fix what the code does today.

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogMessage;
    using Control::Action::Type::ActionOverride;
    using Control::Action::Type::ClientRequests;
    using Bridge::Codec::Flat::Type::BridgeClock;
    using Bridge::Transport::State::BridgeStore;
    using Bridge::Transport::System::BridgeEngine;
    using Bridge::Transport::System::BridgeService;
    using Contract::Bridge::Type::BridgeAction;
    using Contract::Bridge::Type::BridgeMode;
    using Contract::Bridge::Type::BridgeOwner;
    using Contract::Bridge::Type::BridgePhase;
    using Contract::Bridge::Type::BridgeRegion;
    using Bridge::Transport::Type::BridgeStatus;
    using Contract::Bridge::Type::BridgeTick;
    using Services::Settings::State::SettingsStore;
    using Tests::Support::Log::CollectingSink;

    using Contract::Bridge::Type::k_BridgeActionEventSuffix;
    using Contract::Bridge::Type::k_BridgeLayoutHash;
    using Contract::Bridge::Type::k_BridgeLayoutVersion;
    using Contract::Bridge::Type::k_BridgeMagic;
    using Contract::Bridge::Type::k_BridgeNotLocal;
    using Contract::Bridge::Type::k_BridgeRegionPrefix;

    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    constexpr std::uint32_t k_Handle{ 0x0001'0000 };
    constexpr std::uint32_t k_OtherHandle{ 0x0002'0000 };

    // What the bridge asked of the engine, in order.
    struct OverrideCall
    {
        std::size_t Player{ 0 };
        std::optional<ActionOverride> Value{};
    };

    // Engine behind the port: keeps what the bridge writes and gives it back, and the phase and match the test sets.
    class FakeBridgeEngine : public BridgeEngine
    {
    private:
        using BridgePhase = Contract::Bridge::Type::BridgePhase;

        using ActionOverride = Control::Action::Type::ActionOverride;
        using ClientRequests = Control::Action::Type::ClientRequests;

    public:
        FakeBridgeEngine() = default;
        ~FakeBridgeEngine() override = default;

        BridgePhase m_Phase{ BridgePhase::Running };
        std::uint64_t m_EngineStarts{ 1 };
        std::uint64_t m_PausedFrames{ 0 };
        float m_TimeScale{ 1.0f };
        bool m_IsPauseRequested{ false };
        bool m_IsEndRoundRequested{ false };
        std::vector<float> m_ScaleWrites{};
        std::vector<bool> m_PauseWrites{};
        std::vector<bool> m_EndRoundWrites{};
        std::vector<OverrideCall> m_OverrideCalls{};
        std::size_t m_RequestCount{ 0 };

        auto GetPhase() const -> BridgePhase override { return m_Phase; }
        auto GetEngineStarts() const -> std::uint64_t override { return m_EngineStarts; }
        auto GetPausedFrames() const -> std::uint64_t override { return m_PausedFrames; }
        auto GetRequestedTimeScale() const -> float override { return m_TimeScale; }

        auto SetRequestedTimeScale(float scale) -> void override
        {
            m_TimeScale = scale;
            m_ScaleWrites.push_back(scale);
        }

        auto IsPauseRequested() const -> bool override { return m_IsPauseRequested; }

        auto SetPauseRequested(bool value) -> void override
        {
            m_IsPauseRequested = value;
            m_PauseWrites.push_back(value);
        }

        auto SetEndRoundRequested(bool value) -> void override
        {
            m_IsEndRoundRequested = value;
            m_EndRoundWrites.push_back(value);
        }

        auto SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void override
        {
            m_OverrideCalls.push_back(OverrideCall{ .Player = playerIndex, .Value = actionOverride });
        }

        auto RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void override
        {
            static_cast<void>(playerIndex);
            static_cast<void>(requests);
            ++m_RequestCount;
        }

        // return: How many overrides with an action the bridge set for the player.
        auto CountOverrides(std::size_t playerIndex) const -> std::size_t
        {
            return static_cast<std::size_t>(std::ranges::count_if(m_OverrideCalls, [playerIndex](const OverrideCall& call) {
                return call.Player == playerIndex && call.Value.has_value();
            }));
        }
    };

    // Writes the action of a slot the way the client does: the fields first, the decision tick last.
    auto WriteAction(BridgeRegion& region, std::size_t player, std::uint32_t handle, std::uint64_t decision, float yaw,
        std::uint64_t buttons) -> void
    {
        BridgeAction& slot{ region.Actions[player] };
        slot.PlayerHandle = handle;
        slot.Yaw = yaw;
        slot.Buttons = buttons;
        std::atomic_ref<std::uint64_t>(slot.DecisionTick).store(decision, std::memory_order_release);
    }

    // Tells the bridge the client answered up to this decision.
    auto Answer(BridgeRegion& region, std::uint64_t decision) -> void
    {
        std::atomic_ref<std::uint64_t>(region.Control.Client.ActionSequence).store(decision, std::memory_order_release);
    }

    // return: Seconds the call took.
    template <typename Call>
    auto Timed(Call&& call) -> double
    {
        const auto start{ steady_clock::now() };
        call();
        return std::chrono::duration<double>(steady_clock::now() - start).count();
    }
}

class TransportTests : public ::testing::Test
{
protected:
    auto SetUp() -> void override
    {
        ASSERT_TRUE(m_Service.Open());

        // The client maps the region by name, as Python does.
        m_Name = std::string(k_BridgeRegionPrefix) + std::to_string(GetCurrentProcessId());
        const std::wstring wideName(m_Name.begin(), m_Name.end());
        m_Mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, wideName.c_str());
        ASSERT_NE(m_Mapping, nullptr);
        m_Region = static_cast<BridgeRegion*>(MapViewOfFile(m_Mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BridgeRegion)));
        ASSERT_NE(m_Region, nullptr);
    }

    auto TearDown() -> void override
    {
        m_Service.Close();
        if (m_Region != nullptr) UnmapViewOfFile(m_Region);
        if (m_Mapping != nullptr) CloseHandle(m_Mapping);
    }

    CollectingSink m_Sink{};
    Log m_Log{ m_Sink };
    SettingsStore m_SettingsStore{};
    FakeBridgeEngine m_Engine{};
    BridgeStore m_Store{};
    BridgeService m_Service{ m_Log, m_SettingsStore, m_Engine, m_Store };

    std::string m_Name{};
    void* m_Mapping{ nullptr };
    BridgeRegion* m_Region{ nullptr };
    std::uint64_t m_Beat{ 0 };

    // Writes the control block the client asks for and runs one update of the bridge thread.
    auto Connect(BridgeOwner owner, BridgeMode mode, std::uint32_t agentMask) -> void
    {
        auto& client{ m_Region->Control.Client };
        client.RequestOwner = owner;
        client.RequestMode = mode;
        client.RequestPeriod = 4;
        client.RequestLatency = 2;
        client.AgentMask = agentMask;
        client.ClientHeartbeat = ++m_Beat;
        m_Service.Update();
    }

    // Publishes a tick with one player in the slot 0, so the actions of the slot carry its handle.
    auto PublishPlayer(std::uint32_t handle, std::uint8_t localIndex) -> void
    {
        BridgeTick* flat{ m_Service.BeginTick() };
        ASSERT_NE(flat, nullptr);
        flat->Header.PlayerCount = 1;
        flat->Players[0].Handle = handle;
        flat->Players[0].LocalIndex = localIndex;
        m_Service.EndTick(*flat, BridgeClock{});
    }

    auto GetStatus() const -> BridgeStatus
    {
        BridgeStatus status{};
        m_Service.CopyStatus(status);
        return status;
    }

    // return: Whether a log message contains the fragment.
    auto HasLog(std::string_view fragment) const -> bool
    {
        return std::ranges::any_of(m_Sink.m_Messages, [fragment](const LogMessage& message) {
            return message.Message.find(fragment) != std::string::npos;
        });
    }
};

TEST_F(TransportTests, OpenWritesTheHeader)
{
    const auto& artemis{ m_Region->Control.Artemis };
    EXPECT_EQ(artemis.Magic, k_BridgeMagic);
    EXPECT_EQ(artemis.LayoutHash, k_BridgeLayoutHash);
    EXPECT_EQ(artemis.LayoutVersion, k_BridgeLayoutVersion);
    EXPECT_EQ(artemis.RegionSize, sizeof(BridgeRegion));
    EXPECT_EQ(artemis.Pid, static_cast<std::uint32_t>(GetCurrentProcessId()));

    m_Service.Close();
    EXPECT_EQ(artemis.Magic, 0u);
}

TEST_F(TransportTests, NoClientKeepsTheUi)
{
    m_Service.Update();

    const BridgeStatus status{ this->GetStatus() };
    EXPECT_TRUE(status.IsOpen);
    EXPECT_FALSE(status.IsClientConnected);
    EXPECT_EQ(status.Owner, BridgeOwner::UI);
    EXPECT_EQ(status.Mode, BridgeMode::Free);
    EXPECT_EQ(status.AgentMask, 0u);
    EXPECT_EQ(m_Region->Control.Artemis.Phase, BridgePhase::Running);
    EXPECT_EQ(m_Region->Control.Artemis.Heartbeat, 1u);
}

TEST_F(TransportTests, PythonOwnsScaleAndPause)
{
    m_Region->Control.Client.RequestScale = 3.0f;
    m_Region->Control.Client.RequestPause = 1;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    EXPECT_TRUE(this->GetStatus().IsClientConnected);
    ASSERT_EQ(m_Engine.m_ScaleWrites.size(), 1u);
    EXPECT_FLOAT_EQ(m_Engine.m_ScaleWrites.back(), 3.0f);
    ASSERT_EQ(m_Engine.m_PauseWrites.size(), 1u);
    EXPECT_TRUE(m_Engine.m_PauseWrites.back());

    // The client leaves: the UI gets its scale and pause back.
    m_Region->Control.Client.ClientHeartbeat = 0;
    m_Service.Update();

    EXPECT_FLOAT_EQ(m_Engine.m_TimeScale, 1.0f);
    EXPECT_FALSE(m_Engine.m_IsPauseRequested);
    ASSERT_FALSE(m_Engine.m_EndRoundWrites.empty());
    EXPECT_FALSE(m_Engine.m_EndRoundWrites.back());
    EXPECT_FALSE(this->GetStatus().IsClientConnected);
}

TEST_F(TransportTests, EndRoundIsARaise)
{
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    EXPECT_TRUE(m_Engine.m_EndRoundWrites.empty());

    m_Region->Control.Client.RequestEndRound = 1;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    ASSERT_EQ(m_Engine.m_EndRoundWrites.size(), 1u);
    EXPECT_TRUE(m_Engine.m_EndRoundWrites.back());

    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    EXPECT_EQ(m_Engine.m_EndRoundWrites.size(), 1u);

    // A raise while the UI owns the instance is dropped.
    m_Region->Control.Client.RequestEndRound = 2;
    this->Connect(BridgeOwner::UI, BridgeMode::Free, 0);
    EXPECT_EQ(std::ranges::count(m_Engine.m_EndRoundWrites, true), 1);
}

TEST_F(TransportTests, FreeAppliesAtTheLatency)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);
    WriteAction(*m_Region, 0, k_Handle, 4, 1.0f, 5);
    Answer(*m_Region, 4);

    m_Service.Apply(5);
    EXPECT_EQ(m_Engine.CountOverrides(0), 0u);

    m_Service.Apply(6);
    ASSERT_EQ(m_Engine.CountOverrides(0), 1u);
    const ActionOverride& applied{ *m_Engine.m_OverrideCalls.back().Value };
    EXPECT_FLOAT_EQ(applied.Yaw, 1.0f);
    EXPECT_EQ(applied.Buttons, 5u);
    EXPECT_FALSE(applied.LocalIndex.has_value());

    BridgeClock clock{};
    m_Service.CaptureClock(true, clock);
    EXPECT_EQ(clock.AppliedDecisions[0], 4u);
}

TEST_F(TransportTests, HandleMismatchIsCounted)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);
    WriteAction(*m_Region, 0, k_OtherHandle, 4, 1.0f, 5);
    Answer(*m_Region, 4);

    m_Service.Apply(5);
    m_Service.Apply(6);

    EXPECT_EQ(m_Engine.CountOverrides(0), 0u);
    EXPECT_EQ(this->GetStatus().Counters.HandleMismatches, 1u);
}

TEST_F(TransportTests, LockstepTimesOutThenSuspends)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Lockstep, 0x1);
    m_Service.MarkPushed(4);

    EXPECT_GE(Timed([this] { m_Service.Apply(6); }), 0.9);
    EXPECT_EQ(this->GetStatus().Counters.LockstepTimeouts, 1u);

    EXPECT_GE(Timed([this] { m_Service.Apply(7); }), 0.9);
    EXPECT_EQ(this->GetStatus().Counters.LockstepTimeouts, 2u);
    EXPECT_TRUE(this->HasLog("Lockstep suspended"));

    // Suspended: the game does not wait any more.
    m_Service.MarkPushed(8);
    EXPECT_LT(Timed([this] { m_Service.Apply(10); }), 0.2);

    WriteAction(*m_Region, 0, k_Handle, 8, 1.0f, 5);
    Answer(*m_Region, 8);
    m_Service.Apply(11);
    EXPECT_TRUE(this->HasLog("Lockstep resumed"));
}

TEST_F(TransportTests, LockstepAnsweredByTheClient)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Lockstep, 0x1);
    m_Service.MarkPushed(4);

    const std::string eventName{ m_Name + std::string(k_BridgeActionEventSuffix) };
    HANDLE actionEvent{ OpenEventW(EVENT_MODIFY_STATE, FALSE, std::wstring(eventName.begin(), eventName.end()).c_str()) };
    ASSERT_NE(actionEvent, nullptr);

    std::thread client([this, actionEvent] {
        std::this_thread::sleep_for(milliseconds{ 50 });
        WriteAction(*m_Region, 0, k_Handle, 4, 1.0f, 5);
        Answer(*m_Region, 4);
        SetEvent(actionEvent);
    });
    const double seconds{ Timed([this] { m_Service.Apply(6); }) };
    client.join();
    CloseHandle(actionEvent);

    EXPECT_LT(seconds, 0.5);
    ASSERT_EQ(m_Engine.CountOverrides(0), 1u);
    EXPECT_EQ(m_Engine.m_OverrideCalls.back().Value->Buttons, 5u);
    const BridgeStatus status{ this->GetStatus() };
    EXPECT_EQ(status.Counters.LockstepWaits, 1u);
    EXPECT_EQ(status.Counters.LockstepTimeouts, 0u);
}

TEST_F(TransportTests, StopEndsTheWait)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Lockstep, 0x1);
    m_Service.MarkPushed(4);

    std::thread stopper([this] {
        std::this_thread::sleep_for(milliseconds{ 50 });
        m_Service.Stop();
    });
    const double seconds{ Timed([this] { m_Service.Apply(6); }) };
    stopper.join();

    EXPECT_LT(seconds, 0.5);
    EXPECT_EQ(this->GetStatus().Counters.LockstepTimeouts, 0u);
}

TEST_F(TransportTests, ReleasedBotKeepsItsFacing)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);
    WriteAction(*m_Region, 0, k_Handle, 4, 1.0f, 5);
    Answer(*m_Region, 4);
    m_Service.Apply(6);
    ASSERT_EQ(m_Engine.CountOverrides(0), 1u);
    const float pitch{ m_Engine.m_OverrideCalls.back().Value->Pitch };

    // The client lets go of the bot: it stays with the facing it had and nothing else.
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    m_Service.Apply(7);
    const OverrideCall& released{ m_Engine.m_OverrideCalls.back() };
    EXPECT_EQ(released.Player, 0u);
    ASSERT_TRUE(released.Value.has_value());
    EXPECT_FLOAT_EQ(released.Value->Yaw, 1.0f);
    EXPECT_FLOAT_EQ(released.Value->Pitch, pitch);
    EXPECT_EQ(released.Value->Buttons, 0u);
    EXPECT_EQ(released.Value->Lanes, 0u);
    EXPECT_FLOAT_EQ(released.Value->ThrottleForward, 0.0f);
    EXPECT_FLOAT_EQ(released.Value->ThrottleLeft, 0.0f);

    // A local player goes back to its own input.
    this->PublishPlayer(k_Handle, 0);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);
    WriteAction(*m_Region, 0, k_Handle, 8, 1.0f, 5);
    Answer(*m_Region, 8);
    m_Service.Apply(10);
    ASSERT_TRUE(m_Engine.m_OverrideCalls.back().Value.has_value());
    EXPECT_EQ(m_Engine.m_OverrideCalls.back().Value->LocalIndex, std::optional<std::size_t>{ 0 });

    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    m_Service.Apply(11);
    EXPECT_EQ(m_Engine.m_OverrideCalls.back().Player, 0u);
    EXPECT_FALSE(m_Engine.m_OverrideCalls.back().Value.has_value());
}

TEST_F(TransportTests, ClockMarksDecisionTicks)
{
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    m_Service.Apply(8);
    BridgeClock clock{};
    m_Service.CaptureClock(true, clock);
    EXPECT_EQ(clock.ActionTick, 8u);
    EXPECT_TRUE(clock.IsDecision);

    m_Service.Apply(9);
    m_Service.CaptureClock(true, clock);
    EXPECT_EQ(clock.ActionTick, 9u);
    EXPECT_FALSE(clock.IsDecision);
}