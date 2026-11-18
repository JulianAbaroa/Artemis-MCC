#include <gtest/gtest.h>

import Common.Log.System;
import Common.Log.Type;
import Control.Action.Type;
import Services.Layer;
import Engine.Base.Layer;
import Engine.Capture.Layer;
import Engine.Control.Layer;
import Engine.Match.Layer;
import Engine.Render.Layer;
import Engine.Render.State;
import Bridge.Transport.Layer;
import Bridge.Transport.System;
import Bridge.Transport.Type;
import Engine.Control.Input.State;
import Engine.Control.Input.Hook;
import Engine.Base.Memory.System;
import Engine.Base.Hook.System;
import Engine.Control.Simulation.State;
import Engine.Control.Simulation.System;
import Tests.Support.Log;
import std;

// The Layers of the engine projects and of Bridge.Transport (M8 C.2, D242, D243): owners with no callbacks, built over a Services::Layer
// and the Base, and the action source of PlayerActionDetour set by the one that composes them. No MCC in the process of the tests.

extern "C" auto MH_Initialize() -> int;
extern "C" auto MH_Uninitialize() -> int;

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogLevel;
    using Bridge::Transport::System::BridgeEngine;
    using Engine::Control::Input::Hook::PlayerActionDetour;
    using Engine::Control::Input::State::PlayerActionStore;
    using Engine::Base::Memory::System::AOBService;
    using Engine::Control::Simulation::State::BotStore;
    using Engine::Control::Simulation::System::BotService;
    using Tests::Support::Log::CollectingSink;

    volatile int s_HookSink{ 0 };

    // Function the test hooks. Long enough for the trampoline of MinHook.
    __declspec(noinline) auto HookTarget(int value) -> int
    {
        s_HookSink = value;
        s_HookSink = s_HookSink + 1;
        s_HookSink = s_HookSink * 2;
        s_HookSink = s_HookSink - 3;
        return value + 1;
    }

    __declspec(noinline) auto HookReplacement(int value) -> int
    {
        return value + 100;
    }

    // Engine behind the port: it answers with idle values and keeps nothing.
    class IdleBridgeEngine : public BridgeEngine
    {
    private:
        using ActionOverride = Control::Action::Type::ActionOverride;
        using ClientRequests = Control::Action::Type::ClientRequests;

        using BridgePhase = Contract::Bridge::Type::BridgePhase;
        using BridgeActiveSetup = Bridge::Transport::Type::BridgeActiveSetup;
        using BridgeSetupInfo = Bridge::Transport::Type::BridgeSetupInfo;
        using BridgeSetupRequest = Bridge::Transport::Type::BridgeSetupRequest;

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
        auto RequestSetup(const BridgeSetupRequest& request) -> void override { static_cast<void>(request); }
        auto CancelSetupRequest() -> void override {}
        auto GetSetupResult() const -> BridgeSetupInfo override { return BridgeSetupInfo{}; }
        auto GetActiveSetup() const -> BridgeActiveSetup override { return BridgeActiveSetup{}; }
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
    Services::Layer service{};
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

TEST(EngineLayerTests, DisableAndRemoveDetourIgnoreANullAddress)
{
    CollectingSink sink{};
    Log log{ sink };

    ASSERT_EQ(MH_Initialize(), 0);

    using TargetFunction = auto(*)(int) -> int;
    volatile TargetFunction target{ &HookTarget };
    TargetFunction original{ nullptr };

    ASSERT_TRUE(Engine::Base::Hook::System::InstallDetour(reinterpret_cast<void*>(&HookTarget), reinterpret_cast<void*>(&HookReplacement),
        reinterpret_cast<void**>(&original), "[Test]", log));
    EXPECT_EQ(target(1), 101);

    // A null address is MH_ALL_HOOKS for MinHook: the hook of the target must stay on.
    Engine::Base::Hook::System::DisableDetour(nullptr);
    Engine::Base::Hook::System::RemoveDetour(nullptr, "[Test]", log);
    EXPECT_EQ(target(1), 101);

    Engine::Base::Hook::System::DisableDetour(reinterpret_cast<void*>(&HookTarget));
    EXPECT_EQ(target(1), 2);

    Engine::Base::Hook::System::RemoveDetour(reinterpret_cast<void*>(&HookTarget), "[Test]", log);
    MH_Uninitialize();
}

TEST(EngineLayerTests, ChudDetourWithoutTheGameInstallsNothing)
{
    Services::Layer service{};
    Engine::Base::Layer base{ service };
    Engine::Render::Layer render{ service, base };

    // No haloreach.dll in the process: no pass is found, nothing is hooked and the store says so.
    EXPECT_FALSE(render.m_ChudDetour.Install());
    EXPECT_FALSE(render.m_RenderStore.GetHudHook().IsHooked);
    render.m_ChudDetour.Uninstall();
}

TEST(EngineLayerTests, RenderStoreHudFlagAndCallsPerFrame)
{
    Engine::Render::State::RenderStore store{};

    EXPECT_FALSE(store.IsGameHudHidden());
    store.SetGameHudHidden(true);
    EXPECT_TRUE(store.IsGameHudHidden());

    store.CountHudCall();
    store.CountHudCall();
    EXPECT_EQ(store.GetHudHook().CallsPerFrame, 0u);
    store.EndHudFrame();
    EXPECT_EQ(store.GetHudHook().CallsPerFrame, 2u);
    store.EndHudFrame();
    EXPECT_EQ(store.GetHudHook().CallsPerFrame, 0u);
}

TEST(EngineLayerTests, PlayerActionWithoutSourceDoesNotInstall)
{
    CollectingSink sink{};
    Log log{ sink };
    AOBService aobService{ log };
    PlayerActionStore playerActionStore{};
    Services::Layer service{};
    Engine::Base::Layer base{ service };
    BotStore botStore{};
    BotService botService{ log, aobService, botStore, playerActionStore };
    PlayerActionDetour detour{ log, aobService, base.m_MatchSetupService, playerActionStore, botService };

    EXPECT_FALSE(detour.Install());

    ASSERT_EQ(sink.CountOf(LogLevel::Error), 1u);
    const auto error{ std::ranges::find(sink.m_Messages, LogLevel::Error, &Common::Log::Type::LogMessage::Level) };
    ASSERT_NE(error, sink.m_Messages.end());
    EXPECT_EQ(error->Tag, "[PlayerActionDetour]");
    EXPECT_EQ(error->Message, "no action source, hook not installed.");
}