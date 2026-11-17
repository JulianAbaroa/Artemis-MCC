export module Tests.Support.FakeBridge;

import Control.Action.Type;
import Bridge.Transport.Type;
import Bridge.Transport.System;
import std;

// The engine behind the BridgeEngine port for the tests of the bridge and of its users (M6b, D215): it keeps what the bridge writes and gives
// it back, and the phase and match the test sets.

namespace
{
    using Control::Action::Type::ActionOverride;
    using Bridge::Transport::System::BridgeEngine;
    using Contract::Bridge::Type::BridgeSetupResult;
}

export namespace Tests::Support::FakeBridge
{
    // What the bridge asked of the engine, in order.
    struct OverrideCall
    {
        std::size_t Player{ 0 };
        std::optional<ActionOverride> Value{};
    };

    // Engine behind the port: keeps what the bridge writes and gives it back, and the phase and match the test sets.
    // note: m_OnGetEngineStarts runs each time the bridge reads the match, which is once per clock read.
    class FakeBridgeEngine : public BridgeEngine
    {
    private:
        using BridgePhase = Contract::Bridge::Type::BridgePhase;
        using BridgeActiveSetup = Bridge::Transport::Type::BridgeActiveSetup;
        using BridgeSetupInfo = Bridge::Transport::Type::BridgeSetupInfo;
        using BridgeSetupRequest = Bridge::Transport::Type::BridgeSetupRequest;

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
        std::vector<BridgeSetupRequest> m_SetupRequests{};
        std::size_t m_SetupCancels{ 0 };
        BridgeSetupInfo m_SetupInfo{};
        BridgeActiveSetup m_ActiveSetup{};
        std::vector<OverrideCall> m_OverrideCalls{};
        std::size_t m_RequestCount{ 0 };
        mutable std::function<void()> m_OnGetEngineStarts{};

        auto GetPhase() const -> BridgePhase override { return m_Phase; }
        auto GetEngineStarts() const -> std::uint64_t override
        {
            if (m_OnGetEngineStarts) m_OnGetEngineStarts();
            return m_EngineStarts;
        }
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

        auto RequestSetup(const BridgeSetupRequest& request) -> void override
        {
            m_SetupRequests.push_back(request);
            m_SetupInfo.Result = BridgeSetupResult::Pending;
        }

        auto CancelSetupRequest() -> void override { ++m_SetupCancels; }
        auto GetSetupResult() const -> BridgeSetupInfo override { return m_SetupInfo; }
        auto GetActiveSetup() const -> BridgeActiveSetup override { return m_ActiveSetup; }

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
}