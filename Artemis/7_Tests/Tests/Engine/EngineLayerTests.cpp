#include <gtest/gtest.h>

import Common.Log.System;
import Common.Log.Type;
import Control.Action.Type;
import Service.Layer;
import Engine.Base.Layer;
import Engine.Capture.Layer;
import Engine.Control.Layer;
import Engine.Match.Layer;
import Engine.Render.Layer;
import Bridge.Transport.Layer;
import Platform.Bridge.System;
import Platform.Bridge.Type;
import Platform.Input.State;
import Platform.Input.Hook;
import Platform.Memory.System;
import Platform.Simulation.State;
import Platform.Simulation.System;
import Tests.Support.Log;
import std;

// The Layers of the engine projects and of Bridge.Transport (M8 C.2, D242, D243): owners with no callbacks, built over a Service::Layer
// and the Base, and the action source of PlayerActionDetour set by the one that composes them. No MCC in the process of the tests.

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogLevel;
    using Platform::Bridge::System::BridgeEngine;
    using Platform::Input::Hook::PlayerActionDetour;
    using Platform::Input::State::PlayerActionStore;
    using Platform::Memory::System::AOBService;
    using Platform::Simulation::State::BotStore;
    using Platform::Simulation::System::BotService;
    using Tests::Support::Log::CollectingSink;

    // Engine behind the port: it answers with idle values and keeps nothing.
    class IdleBridgeEngine : public BridgeEngine
    {
    private:
        using ActionOverride = Control::Action::Type::ActionOverride;
        using ClientRequests = Control::Action::Type::ClientRequests;

        using BridgePhase = Platform::Bridge::Type::BridgePhase;

    public:
        IdleBridgeEngine() = default;
        ~IdleBridgeEngine() override = default;

        auto GetPhase() const -> BridgePhase override { return BridgePhase::Idle; }
        auto GetEngineStarts() const -> std::uint64_t override { return 0; }
        auto GetPausedFrames() const -> std::uint64_t override { return 0; }
        auto GetRequestedTimeScale() const -> float override { return 1.0f; }
        auto SetRequestedTimeScale(float scale) -> void override { static_cast<void>(scale); }
        auto IsPauseRequested() const -> bool override { return false; }
        auto SetPauseRequested(bool value) -> void override { static_cast<void>(value); }
        auto SetEndRoundRequested(bool value) -> void override { static_cast<void>(value); }

        auto SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void override
        {
            static_cast<void>(playerIndex);
            static_cast<void>(actionOverride);
        }

        auto RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void override
        {
            static_cast<void>(playerIndex);
            static_cast<void>(requests);
        }
    };
}

TEST(EngineLayerTests, BuildsAndDestroysWithoutTheGame)
{
    Service::Layer service{};
    IdleBridgeEngine engine{};
    {
        Engine::Base::Layer base{ service };
        Engine::Capture::Layer capture{ service, base };
        Engine::Control::Layer control{ service, base };
        Engine::Match::Layer match{ service, base };
        Engine::Render::Layer render{ service, base };
        Bridge::Transport::Layer transport{ service, engine };

        // Nothing is installed or opened by building them: the lifecycle never ran.
        EXPECT_FALSE(base.m_LifecycleStore.IsRunning());
    }
    SUCCEED();
}

TEST(EngineLayerTests, PlayerActionWithoutSourceDoesNotInstall)
{
    CollectingSink sink{};
    Log log{ sink };
    AOBService aobService{ log };
    PlayerActionStore playerActionStore{};
    BotStore botStore{};
    BotService botService{ log, aobService, botStore };
    PlayerActionDetour detour{ log, aobService, playerActionStore, botService };

    EXPECT_FALSE(detour.Install());

    ASSERT_EQ(sink.CountOf(LogLevel::Error), 1u);
    const auto error{ std::ranges::find(sink.m_Messages, LogLevel::Error, &Common::Log::Type::LogMessage::Level) };
    ASSERT_NE(error, sink.m_Messages.end());
    EXPECT_EQ(error->Tag, "[PlayerActionDetour]");
    EXPECT_EQ(error->Message, "no action source, hook not installed.");
}