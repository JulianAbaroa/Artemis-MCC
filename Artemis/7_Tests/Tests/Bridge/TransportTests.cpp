#include <gtest/gtest.h>
#include <windows.h>

import Common.Log.System;
import Common.Log.Type;
import Control.Client.Type;
import Services.Settings.State;
import Bridge.Codec.Flat.Type;
import Bridge.Transport.Type;
import Bridge.Transport.State;
import Bridge.Transport.System;
import Tests.Support.FakeBridge;
import Tests.Support.Log;
import std;

// Bridge.Transport (M6b, D215): the real BridgeService over its shared memory and events, with a fake engine behind the BridgeEngine port.
// The test acts as the Python client: it writes the control block and the actions of the region and calls Update, Apply, BeginTick and
// EndTick the way the threads of the game do. The tests fix what the code does today.

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogMessage;
    using Bridge::Codec::Flat::Type::BridgeClock;
    using Bridge::Transport::State::BridgeStore;
    using Bridge::Transport::System::BridgeEngine;
    using Bridge::Transport::System::BridgeService;
    using Contract::Bridge::Type::BridgeAction;
    using Contract::Bridge::Type::BridgeAgentInput;
    using Contract::Bridge::Type::BridgeMode;
    using Contract::Bridge::Type::BridgeOwner;
    using Contract::Bridge::Type::BridgePhase;
    using Contract::Bridge::Type::BridgeRoster;
    using Contract::Bridge::Type::BridgeSetupResult;
    using Contract::Bridge::Type::BridgeRegion;
    using Bridge::Transport::Type::BridgeActiveSetup;
    using Bridge::Transport::Type::BridgeSetupInfo;
    using Bridge::Transport::Type::BridgeSetupRequest;
    using Bridge::Transport::Type::BridgeStatus;
    using Contract::Bridge::Type::BridgeTick;
    using Services::Settings::State::SettingsStore;
    using Tests::Support::FakeBridge::FakeBridgeEngine;
    using Tests::Support::FakeBridge::AgentCall;
    using Tests::Support::Log::CollectingSink;

    using Contract::Bridge::Type::k_BridgeActionEventSuffix;
    using Contract::Bridge::Type::k_BridgeLayoutHash;
    using Contract::Bridge::Type::k_BridgeLayoutVersion;
    using Contract::Bridge::Type::k_BridgeMapNameSize;
    using Contract::Bridge::Type::k_BridgeMagic;
    using Contract::Bridge::Type::k_BridgeNotLocal;
    using Contract::Bridge::Type::k_BridgePresetNameSize;
    using Contract::Bridge::Type::k_BridgeSetupMap;
    using Contract::Bridge::Type::k_BridgeSetupPreset;
    using Contract::Bridge::Type::k_BridgeSetupReasonSize;
    using Contract::Bridge::Type::k_BridgeSetupRoster;
    using Contract::Bridge::Type::k_BridgeRegionPrefix;

    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    constexpr std::uint32_t k_Handle{ 0x0001'0000 };
    constexpr std::uint32_t k_OtherHandle{ 0x0002'0000 };

    // Writes the action of a slot the way the client does: the fields first, the decision tick last.
    auto WriteAction(BridgeRegion& region, std::size_t player, std::uint32_t handle, std::uint64_t decision, const BridgeAgentInput& input) -> void
    {
        BridgeAction& slot{ region.Actions[player] };
        slot.PlayerHandle = handle;
        slot.Input = input;
        std::atomic_ref<std::uint64_t>(slot.DecisionTick).store(decision, std::memory_order_release);
    }

    // return: An input that moves forward and holds the controls of Level (bit 0 Jump, bit 2 Fire).
    auto MakeInput(float forward, std::uint32_t level) -> BridgeAgentInput
    {
        return BridgeAgentInput{ .MoveForward = forward, .Level = level, .Edge = level };
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
    EXPECT_FALSE(status.Control.IsClientConnected);
    EXPECT_EQ(status.Control.Owner, BridgeOwner::UI);
    EXPECT_EQ(status.Control.Mode, BridgeMode::Free);
    EXPECT_EQ(status.Control.AgentMask, 0u);
    EXPECT_EQ(m_Region->Control.Artemis.Phase, BridgePhase::Running);
    EXPECT_EQ(m_Region->Control.Artemis.Heartbeat, 1u);
}

TEST_F(TransportTests, PythonOwnsScaleAndPause)
{
    m_Region->Control.Client.RequestScale = 3.0f;
    m_Region->Control.Client.RequestPause = 1;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    EXPECT_TRUE(this->GetStatus().Control.IsClientConnected);
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
    EXPECT_FALSE(this->GetStatus().Control.IsClientConnected);
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

namespace
{
    auto Text(const auto& bytes) -> std::string
    {
        return std::string(bytes.begin(), std::ranges::find(bytes, std::uint8_t{ 0 }));
    }
}

TEST_F(TransportTests, ASetupIsARaiseWithItsFieldsAndIsEchoedWithItsResult)
{
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    EXPECT_TRUE(m_Engine.m_SetupRequests.empty());

    auto& client{ m_Region->Control.Client };
    const std::string_view preset{ "artemis_ctf_fast" };
    const std::string_view map{ "Reach Bus Mountain Pass Reach Bus CTF" };
    std::ranges::copy(preset, client.RequestSetupPreset.begin());
    std::ranges::copy(map, client.RequestSetupMap.begin());
    client.RequestSetupRoster.Teams[0] = 3;
    client.RequestSetupRoster.Teams[1] = 4;
    client.RequestSetupRoster.Auto = 2;
    client.RequestSetupFields = k_BridgeSetupPreset | k_BridgeSetupMap | k_BridgeSetupRoster;
    client.RequestSetup = 1;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    ASSERT_EQ(m_Engine.m_SetupRequests.size(), 1u);
    const BridgeSetupRequest& request{ m_Engine.m_SetupRequests.back() };
    EXPECT_EQ(request.Preset, std::string{ preset });
    EXPECT_EQ(request.Map, std::string{ map });
    ASSERT_TRUE(request.Roster.has_value());
    EXPECT_EQ(request.Roster->Teams[0], 3);
    EXPECT_EQ(request.Roster->Teams[1], 4);
    EXPECT_EQ(request.Roster->Auto, 2);
    EXPECT_EQ(m_Region->Control.Artemis.SetupSequence, 1u);
    EXPECT_EQ(m_Region->Control.Artemis.SetupResult, BridgeSetupResult::Pending);

    // The same counter is not a new request. The result, the round, the reason and the active setup follow the engine.
    m_Engine.m_SetupInfo = BridgeSetupInfo{ .Result = BridgeSetupResult::Applied, .Round = 3, .Reason = "partial: 2 of 4 bots" };
    BridgeRoster active{};
    active.Teams[1] = 5;
    active.Auto = 1;
    m_Engine.m_ActiveSetup = BridgeActiveSetup{ .Preset = std::string{ preset }, .Map = std::string{ map }, .Roster = active };
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    EXPECT_EQ(m_Engine.m_SetupRequests.size(), 1u);
    const auto& artemis{ m_Region->Control.Artemis };
    EXPECT_EQ(artemis.SetupResult, BridgeSetupResult::Applied);
    EXPECT_EQ(artemis.SetupRound, 3);
    EXPECT_EQ(Text(artemis.SetupReason), "partial: 2 of 4 bots");
    EXPECT_EQ(Text(artemis.ActivePreset), preset);
    EXPECT_EQ(Text(artemis.ActiveMap), map);
    EXPECT_EQ(artemis.ActiveHasRoster, 1);
    EXPECT_EQ(artemis.ActiveRoster.Teams[1], 5);
    EXPECT_EQ(artemis.ActiveRoster.Auto, 1);

    // An empty preset with its bit set asks for the base values, and the bits that are not set stay empty.
    client.RequestSetupPreset.fill(0);
    client.RequestSetupFields = k_BridgeSetupPreset;
    client.RequestSetup = 2;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    ASSERT_EQ(m_Engine.m_SetupRequests.size(), 2u);
    EXPECT_EQ(m_Engine.m_SetupRequests.back().Preset, std::string{});
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Map.has_value());
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Roster.has_value());
    EXPECT_EQ(m_Region->Control.Artemis.SetupSequence, 2u);
}

TEST_F(TransportTests, TheFieldMaskDecidesWhichFieldsTheRequestCarries)
{
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    // Bytes in the fields whose bit is not set are not part of the request.
    auto& client{ m_Region->Control.Client };
    client.RequestSetupPreset[0] = 'p';
    client.RequestSetupMap[0] = 'm';
    client.RequestSetupRoster.Auto = 7;

    client.RequestSetupFields = k_BridgeSetupMap;
    client.RequestSetup = 1;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    ASSERT_EQ(m_Engine.m_SetupRequests.size(), 1u);
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Preset.has_value());
    EXPECT_EQ(m_Engine.m_SetupRequests.back().Map, std::string{ "m" });
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Roster.has_value());

    client.RequestSetupFields = k_BridgeSetupRoster;
    client.RequestSetup = 2;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    ASSERT_EQ(m_Engine.m_SetupRequests.size(), 2u);
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Preset.has_value());
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Map.has_value());
    ASSERT_TRUE(m_Engine.m_SetupRequests.back().Roster.has_value());
    EXPECT_EQ(m_Engine.m_SetupRequests.back().Roster->Auto, 7);

    // A raise with no field is a request that changes nothing, and still counts as one.
    client.RequestSetupFields = 0;
    client.RequestSetup = 3;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    ASSERT_EQ(m_Engine.m_SetupRequests.size(), 3u);
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Preset.has_value());
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Map.has_value());
    EXPECT_FALSE(m_Engine.m_SetupRequests.back().Roster.has_value());
}

TEST_F(TransportTests, ANameThatFillsItsFieldHasNoEndZeroAndAReasonOfSixtyFourCharactersFillsItsField)
{
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    auto& client{ m_Region->Control.Client };
    const std::string map(k_BridgeMapNameSize, 'm');
    const std::string preset(k_BridgePresetNameSize, 'p');
    std::ranges::copy(map, client.RequestSetupMap.begin());
    std::ranges::copy(preset, client.RequestSetupPreset.begin());
    client.RequestSetupFields = k_BridgeSetupPreset | k_BridgeSetupMap;
    client.RequestSetup = 1;
    m_Engine.m_ActiveSetup.Map = map;
    m_Engine.m_SetupInfo.Reason = std::string(k_BridgeSetupReasonSize, 'r');
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    ASSERT_EQ(m_Engine.m_SetupRequests.size(), 1u);
    EXPECT_EQ(m_Engine.m_SetupRequests.back().Map, map);
    EXPECT_EQ(m_Engine.m_SetupRequests.back().Preset, preset);

    const auto& artemis{ m_Region->Control.Artemis };
    EXPECT_EQ(std::string(artemis.ActiveMap.begin(), artemis.ActiveMap.end()), map);
    EXPECT_EQ(std::string(artemis.SetupReason.begin(), artemis.SetupReason.end()), std::string(k_BridgeSetupReasonSize, 'r'));
}

TEST_F(TransportTests, ASetupRaiseWhileTheUIOwnsTheInstanceIsDroppedButEchoed)
{
    auto& client{ m_Region->Control.Client };
    client.RequestSetupPreset[0] = 'a';
    client.RequestSetupFields = k_BridgeSetupPreset;
    client.RequestSetup = 1;
    this->Connect(BridgeOwner::UI, BridgeMode::Free, 0);

    EXPECT_TRUE(m_Engine.m_SetupRequests.empty());
    EXPECT_EQ(m_Region->Control.Artemis.SetupSequence, 1u);
}

TEST_F(TransportTests, TheSetupRequestIsDroppedOnceWhenPythonLosesTheInstance)
{
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    EXPECT_EQ(m_Engine.m_SetupCancels, 0u);

    m_Region->Control.Client.RequestSetupPreset[0] = 'a';
    m_Region->Control.Client.RequestSetupFields = k_BridgeSetupPreset;
    m_Region->Control.Client.RequestSetup = 1;
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    EXPECT_EQ(m_Engine.m_SetupCancels, 0u);

    this->Connect(BridgeOwner::UI, BridgeMode::Free, 0);
    EXPECT_EQ(m_Engine.m_SetupCancels, 1u);
}

TEST_F(TransportTests, FreeAppliesAtTheLatency)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);
    m_Region->Control.Client.RequestTurnLimit = 0.25f;
    WriteAction(*m_Region, 0, k_Handle, 4, MakeInput(1.0f, 5));
    Answer(*m_Region, 4);

    m_Service.Apply(5);
    EXPECT_EQ(m_Engine.CountInputs(0), 0u);

    m_Service.Apply(6);
    ASSERT_EQ(m_Engine.CountInputs(0), 1u);
    const AgentCall& applied{ m_Engine.m_AgentCalls.back() };
    EXPECT_FLOAT_EQ(applied.Input->MoveForward, 1.0f);
    EXPECT_EQ(applied.Input->Level, 5u);
    EXPECT_EQ(applied.Input->Edge, 5u);
    EXPECT_EQ(applied.PlayerHandle, k_Handle);

    // The window is the latency (d) and the turn limit goes with it, as the control block gave them.
    EXPECT_EQ(applied.WindowTicks, 2u);
    EXPECT_FLOAT_EQ(applied.TurnLimit, 0.25f);

    BridgeClock clock{};
    m_Service.CaptureClock(true, clock);
    EXPECT_EQ(clock.AppliedDecisions[0], 4u);
}

TEST_F(TransportTests, HandleMismatchIsCounted)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);
    WriteAction(*m_Region, 0, k_OtherHandle, 4, MakeInput(1.0f, 5));
    Answer(*m_Region, 4);

    m_Service.Apply(5);
    m_Service.Apply(6);

    EXPECT_EQ(m_Engine.CountInputs(0), 0u);
    EXPECT_EQ(this->GetStatus().Counters.HandleMismatches, 1u);
}

// "Pushed" is a decision queued for the AI thread (RV.1), not one that reached Python: the game waits only for decisions up to the one marked.
// A decision the producer lost is never marked, so the game does not wait for it. One that is marked and stays unanswered costs a wait of
// k_LockstepTimeout, and two in a row suspend Lockstep until the client answers.
TEST_F(TransportTests, LockstepWaitsForAQueuedDecisionAndSuspendsAfterTwoTimeouts)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Lockstep, 0x1);

    // Nothing was queued yet: no wait, no timeout.
    EXPECT_LT(Timed([this] { m_Service.Apply(6); }), 0.2);
    EXPECT_EQ(this->GetStatus().Counters.LockstepTimeouts, 0u);

    m_Service.MarkPushed(4);

    EXPECT_GE(Timed([this] { m_Service.Apply(6); }), 0.9);
    EXPECT_EQ(this->GetStatus().Counters.LockstepTimeouts, 1u);

    EXPECT_GE(Timed([this] { m_Service.Apply(7); }), 0.9);
    EXPECT_EQ(this->GetStatus().Counters.LockstepTimeouts, 2u);
    EXPECT_TRUE(this->HasLog("Lockstep suspended"));

    // Suspended: the game does not wait any more.
    m_Service.MarkPushed(8);
    EXPECT_LT(Timed([this] { m_Service.Apply(10); }), 0.2);

    WriteAction(*m_Region, 0, k_Handle, 8, MakeInput(1.0f, 5));
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
        WriteAction(*m_Region, 0, k_Handle, 4, MakeInput(1.0f, 5));
        Answer(*m_Region, 4);
        SetEvent(actionEvent);
    });
    const double seconds{ Timed([this] { m_Service.Apply(6); }) };
    client.join();
    CloseHandle(actionEvent);

    EXPECT_LT(seconds, 0.5);
    ASSERT_EQ(m_Engine.CountInputs(0), 1u);
    EXPECT_EQ(m_Engine.m_AgentCalls.back().Input->Level, 5u);
    const BridgeStatus status{ this->GetStatus() };
    EXPECT_EQ(status.Counters.LockstepWaits, 1u);
    EXPECT_EQ(status.Counters.LockstepTimeouts, 0u);
    EXPECT_GT(m_Service.TakeLockstepWaitMax(), 0u);
    EXPECT_EQ(m_Service.TakeLockstepWaitMax(), 0u);
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

TEST_F(TransportTests, ReleasedBotStandsStillAndALocalGetsItsInputBack)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);
    WriteAction(*m_Region, 0, k_Handle, 4, MakeInput(1.0f, 5));
    Answer(*m_Region, 4);
    m_Service.Apply(6);
    ASSERT_EQ(m_Engine.CountInputs(0), 1u);

    // The client lets go of the bot: it gets an input that asks for nothing, so it keeps its facing and stops (repetition without look).
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    m_Service.Apply(7);
    const AgentCall& released{ m_Engine.m_AgentCalls.back() };
    EXPECT_EQ(released.Player, 0u);
    ASSERT_TRUE(released.Input.has_value());
    EXPECT_EQ(released.PlayerHandle, k_Handle);
    EXPECT_FLOAT_EQ(released.Input->MoveForward, 0.0f);
    EXPECT_FLOAT_EQ(released.Input->MoveLeft, 0.0f);
    EXPECT_FLOAT_EQ(released.Input->YawDelta, 0.0f);
    EXPECT_FLOAT_EQ(released.Input->PitchDelta, 0.0f);
    EXPECT_EQ(released.Input->Level, 0u);
    EXPECT_EQ(released.Input->Edge, 0u);
    EXPECT_EQ(released.WindowTicks, 1u);

    // A local player goes back to its own input.
    this->PublishPlayer(k_Handle, 0);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);
    WriteAction(*m_Region, 0, k_Handle, 8, MakeInput(1.0f, 5));
    Answer(*m_Region, 8);
    m_Service.Apply(10);
    ASSERT_TRUE(m_Engine.m_AgentCalls.back().Input.has_value());

    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);
    m_Service.Apply(11);
    EXPECT_EQ(m_Engine.m_AgentCalls.back().Player, 0u);
    EXPECT_FALSE(m_Engine.m_AgentCalls.back().Input.has_value());
}

TEST_F(TransportTests, AnInputWithValuesOutOfRangeIsMadeSaneAndCounted)
{
    this->PublishPlayer(k_Handle, k_BridgeNotLocal);
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0x1);

    BridgeAgentInput broken{ .MoveForward = 3.0f, .MoveLeft = std::numeric_limits<float>::quiet_NaN(), .YawDelta = 0.5f,
        .PitchDelta = std::numeric_limits<float>::infinity(), .Level = 0xFFFF'FFFFu, .Edge = 0x8000'0001u };
    WriteAction(*m_Region, 0, k_Handle, 4, broken);
    Answer(*m_Region, 4);
    m_Service.Apply(6);

    ASSERT_EQ(m_Engine.CountInputs(0), 1u);
    const auto& input{ *m_Engine.m_AgentCalls.back().Input };
    EXPECT_FLOAT_EQ(input.MoveForward, 1.0f);
    EXPECT_FLOAT_EQ(input.MoveLeft, 0.0f);
    EXPECT_FLOAT_EQ(input.YawDelta, 0.5f);
    EXPECT_FLOAT_EQ(input.PitchDelta, 0.0f);
    EXPECT_EQ(input.Level, 0x1FFFu);
    EXPECT_EQ(input.Edge, 0x1u);
    EXPECT_EQ(this->GetStatus().Counters.RejectedActions, 1u);

    // A sane input is not counted.
    WriteAction(*m_Region, 0, k_Handle, 8, MakeInput(-0.5f, 0x1FFFu));
    Answer(*m_Region, 8);
    m_Service.Apply(10);
    EXPECT_EQ(m_Engine.CountInputs(0), 2u);
    EXPECT_EQ(this->GetStatus().Counters.RejectedActions, 1u);
}

// The imitation block (D364): the clock carries the possessed player and the last closed window together, and EndTick writes both into the header. The window
// closes where one ends: every d ticks counted from the decision tick.
TEST_F(TransportTests, TheClockCarriesTheLastWindowAndTheWindowsCloseEveryLatencyTicks)
{
    this->Connect(BridgeOwner::Python, BridgeMode::Free, 0);

    for (std::uint64_t tick{ 1 }; tick <= 8; ++tick) m_Service.Apply(tick);
    EXPECT_EQ(m_Engine.m_WindowCloses, 4u);

    m_Engine.m_Imitation.PlayerHandle = k_Handle;
    m_Engine.m_Imitation.Input = Control::Client::Type::AgentInput{ .MoveForward = 1.0f, .YawDelta = -0.5f, .Level = 4, .Edge = 5 };
    BridgeClock clock{};
    m_Service.CaptureClock(true, clock);
    EXPECT_EQ(clock.PossessedPlayer, k_Handle);
    EXPECT_FLOAT_EQ(clock.PossessedInput.MoveForward, 1.0f);
    EXPECT_FLOAT_EQ(clock.PossessedInput.YawDelta, -0.5f);
    EXPECT_EQ(clock.PossessedInput.Level, 4u);
    EXPECT_EQ(clock.PossessedInput.Edge, 5u);

    BridgeTick* flat{ m_Service.BeginTick() };
    ASSERT_NE(flat, nullptr);
    m_Service.EndTick(*flat, clock);
    EXPECT_EQ(flat->Header.PossessedPlayer, k_Handle);
    EXPECT_EQ(flat->Header.PossessedInput.Edge, 5u);

    // Nobody possessed: no handle and a zero input, together.
    m_Engine.m_Imitation = {};
    m_Service.CaptureClock(true, clock);
    EXPECT_EQ(clock.PossessedPlayer, 0xFFFF'FFFFu);
    EXPECT_EQ(clock.PossessedInput.Level, 0u);
}

TEST_F(TransportTests, TheLayoutIsTheOneOfTheBridgeV2)
{
    EXPECT_EQ(k_BridgeLayoutHash, 0x4B4E4E552E4DF04FULL);
    EXPECT_EQ(sizeof(BridgeAgentInput), 24u);
    EXPECT_EQ(sizeof(BridgeAction), 40u);
    EXPECT_EQ(offsetof(BridgeAction, Input), 16u);
    EXPECT_EQ(sizeof(BridgeRegion), 492992u);
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