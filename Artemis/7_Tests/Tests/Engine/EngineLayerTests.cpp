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
import Engine.Render.System;
import Engine.Render.Type;
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

TEST(EngineLayerTests, FirstPersonDetourWithoutTheGameInstallsNothing)
{
    Services::Layer service{};
    Engine::Base::Layer base{ service };
    Engine::Render::Layer render{ service, base };

    // No haloreach.dll in the process: neither function is found, nothing is hooked and the store says so.
    EXPECT_FALSE(render.m_FirstPersonDetour.Install());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonModelHooked());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonUnitHooked());
    render.m_FirstPersonDetour.Uninstall();
}

TEST(EngineLayerTests, FirstPersonDecisionsOnlyChangeUserZero)
{
    using Engine::Render::Hook::FirstPersonDetour;

    // The model pass: skipped for user 0 with the flag or with Drive, never for another user.
    EXPECT_FALSE(FirstPersonDetour::IsFirstPersonModelSkipped(0, false, false));
    EXPECT_TRUE(FirstPersonDetour::IsFirstPersonModelSkipped(0, true, false));
    EXPECT_TRUE(FirstPersonDetour::IsFirstPersonModelSkipped(0, false, true));
    EXPECT_TRUE(FirstPersonDetour::IsFirstPersonModelSkipped(0, true, true));
    EXPECT_FALSE(FirstPersonDetour::IsFirstPersonModelSkipped(1, true, true));
    EXPECT_FALSE(FirstPersonDetour::IsFirstPersonModelSkipped(1, false, false));

    // The unit test: false only for user 0 with Drive when the game said true. The flag alone never changes it.
    EXPECT_TRUE(FirstPersonDetour::IsThirdPersonShown(0, true, true));
    EXPECT_FALSE(FirstPersonDetour::IsThirdPersonShown(0, true, false));
    EXPECT_FALSE(FirstPersonDetour::IsThirdPersonShown(0, false, true));
    EXPECT_FALSE(FirstPersonDetour::IsThirdPersonShown(1, true, true));
}

TEST(EngineLayerTests, RenderStoreFirstPersonFlagAndThirdPersonFrame)
{
    Engine::Render::State::RenderStore store{};

    EXPECT_FALSE(store.IsFirstPersonHidden());
    store.SetFirstPersonHidden(true);
    EXPECT_TRUE(store.IsFirstPersonHidden());

    // The third person count of a frame is read after the frame is closed, and starts again.
    EXPECT_FALSE(store.WasThirdPersonLastFrame());
    store.CountThirdPerson();
    EXPECT_FALSE(store.WasThirdPersonLastFrame());
    store.EndHudFrame();
    EXPECT_TRUE(store.WasThirdPersonLastFrame());
    store.EndHudFrame();
    EXPECT_FALSE(store.WasThirdPersonLastFrame());
}

TEST(EngineLayerTests, FirstPersonCallerIsClassifiedByFunctionRange)
{
    using Engine::Render::Hook::FirstPersonDetour;
    using Engine::Render::Type::FirstPersonCaller;

    // The first byte and the last byte of each function belong to it, the byte before and the one after do not.
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26DA08), FirstPersonCaller::Pass1);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26DA08 + 497), FirstPersonCaller::Pass1);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26DA08 - 1), FirstPersonCaller::Other);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26DA08 + 498), FirstPersonCaller::Other);

    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26E2A0), FirstPersonCaller::Func26E2A0);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26E2A0 + 604), FirstPersonCaller::Func26E2A0);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26E2A0 + 605), FirstPersonCaller::Other);

    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26EA78), FirstPersonCaller::ViewPhase);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26EA78 + 787), FirstPersonCaller::ViewPhase);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26EA78 - 1), FirstPersonCaller::Other);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x26EA78 + 788), FirstPersonCaller::Other);

    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x277D00), FirstPersonCaller::Func277D00);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x277D00 + 542), FirstPersonCaller::Func277D00);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0x277D00 + 543), FirstPersonCaller::Other);

    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0), FirstPersonCaller::Other);
    EXPECT_EQ(FirstPersonDetour::ClassifyCaller(0xFFFFFFFF), FirstPersonCaller::Other);
}

TEST(EngineLayerTests, RenderStoreFirstPersonSampleAndOneShotProbe)
{
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::FirstPersonSample;

    Engine::Render::State::RenderStore store{};

    // A sample is kept in the slot of its caller, with the calls of the last closed frame.
    FirstPersonSample sample{};
    sample.HasSample = true;
    sample.Caller = FirstPersonCaller::ViewPhase;
    sample.Pass = 3;
    sample.Depth.HasDepth = true;
    sample.Depth.Resource = 0x1234;
    store.SetFirstPersonSample(sample);

    store.CountFirstPersonCall(FirstPersonCaller::ViewPhase);
    store.CountFirstPersonCall(FirstPersonCaller::ViewPhase);
    store.CountFirstPersonCall(FirstPersonCaller::Other);
    EXPECT_EQ(store.GetFirstPersonCallerCount(), 0u);
    store.EndHudFrame();
    EXPECT_EQ(store.GetFirstPersonCallerCount(), 2u);
    EXPECT_EQ(store.GetFirstPersonCalls(FirstPersonCaller::ViewPhase), 2u);

    const FirstPersonSample read{ store.GetFirstPersonSample(FirstPersonCaller::ViewPhase) };
    EXPECT_TRUE(read.HasSample);
    EXPECT_EQ(read.Pass, 3u);
    EXPECT_EQ(read.Depth.Resource, 0x1234u);
    EXPECT_EQ(read.CallsPerFrame, 2u);
    EXPECT_FALSE(store.GetFirstPersonSample(FirstPersonCaller::Pass1).HasSample);

    // The probe is armed at the Present after the request, and ends at the Present that closes a frame with calls. One shot.
    EXPECT_FALSE(store.IsFirstPersonProbePending());
    store.RequestFirstPersonProbe();
    EXPECT_TRUE(store.IsFirstPersonProbePending());
    EXPECT_FALSE(store.IsFirstPersonProbeArmed());

    EXPECT_FALSE(store.StepFirstPersonProbe());
    EXPECT_TRUE(store.IsFirstPersonProbeArmed());
    EXPECT_FALSE(store.GetFirstPersonSample(FirstPersonCaller::ViewPhase).HasSample);   // cleared when armed

    store.EndHudFrame();    // a frame with no first person call: keeps waiting
    EXPECT_FALSE(store.StepFirstPersonProbe());
    EXPECT_TRUE(store.IsFirstPersonProbeArmed());

    store.CountFirstPersonCall(FirstPersonCaller::Func26E2A0);
    store.EndHudFrame();
    EXPECT_TRUE(store.StepFirstPersonProbe());
    EXPECT_FALSE(store.IsFirstPersonProbePending());

    EXPECT_FALSE(store.StepFirstPersonProbe());
    store.RequestFirstPersonProbe();
    EXPECT_TRUE(store.IsFirstPersonProbePending());
}

TEST(EngineLayerTests, ObserverCameraDetourWithoutTheGameInstallsNothing)
{
    Services::Layer service{};
    Engine::Base::Layer base{ service };

    // No haloreach.dll in the process: the function is not found and nothing is hooked. Uninstall of a hook that was never installed is a no-op.
    EXPECT_FALSE(base.m_ObserverCameraDetour.Install());
    base.m_ObserverCameraDetour.Uninstall();

    // The override still works without the hook: the store only carries the pose.
    EXPECT_TRUE(base.m_GameCameraService.SetOverride({ 1.0f, 2.0f, 3.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }));
    EXPECT_TRUE(base.m_CameraOverrideStore.IsActive());
    base.m_GameCameraService.ClearOverride();
    EXPECT_FALSE(base.m_CameraOverrideStore.IsActive());
}

TEST(EngineLayerTests, BeforeHudDrawsOncePerPresentFrame)
{
    using Engine::Render::State::RenderStore;
    using Engine::Render::System::RenderService;
    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::HudHookInfo;

    RenderStore store{};

    // The mark belongs to the Present frame in progress: set before the Present handlers, it is gone after the Present.
    EXPECT_FALSE(store.IsBeforeHudMarked());
    store.MarkBeforeHud();
    store.MarkBeforeHud();
    EXPECT_TRUE(store.IsBeforeHudMarked());
    store.NextPresentFrame();
    EXPECT_FALSE(store.IsBeforeHudMarked());
    store.NextPresentFrame();
    EXPECT_FALSE(store.IsBeforeHudMarked());

    // The status of a frame is shown once the frame completes, and the next frame starts with no HUD draw.
    store.SetBeforeHudStatus(BeforeHudStatus::Drawn);
    EXPECT_EQ(store.GetBeforeHudStatus(), BeforeHudStatus::NoHudDraw);
    store.NextPresentFrame();
    EXPECT_EQ(store.GetBeforeHudStatus(), BeforeHudStatus::Drawn);
    store.NextPresentFrame();
    EXPECT_EQ(store.GetBeforeHudStatus(), BeforeHudStatus::NoHudDraw);

    HudHookInfo sample{};
    sample.HasSample = true;
    sample.IsSameThread = true;
    sample.HasTarget = true;
    sample.IsBackBuffer = true;
    sample.IsSameDevice = true;
    sample.HasViewport = true;
    sample.Width = 1920;
    sample.Height = 1080;
    sample.ViewportWidth = 1920.0f;
    sample.ViewportHeight = 1080.0f;

    EXPECT_FALSE(RenderService::FindBeforeHudBlocker(sample, true, 1920, 1080).has_value());
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, false, 1920, 1080), BeforeHudStatus::NotReady);
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 1280, 720), BeforeHudStatus::TargetMismatch);
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 0, 0), BeforeHudStatus::TargetMismatch);

    // The bound target must be the current back buffer of the store, of its device, and the viewport the whole target.
    sample.IsBackBuffer = false;
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 1920, 1080), BeforeHudStatus::NotBackBuffer);
    sample.IsBackBuffer = true;

    sample.IsSameDevice = false;
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 1920, 1080), BeforeHudStatus::OtherDevice);
    sample.IsSameDevice = true;

    sample.ViewportWidth = 960.0f;
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 1920, 1080), BeforeHudStatus::SplitViewport);
    sample.ViewportWidth = 1920.0f;
    sample.ViewportX = 4.0f;
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 1920, 1080), BeforeHudStatus::SplitViewport);
    sample.ViewportX = 0.0f;
    sample.HasViewport = false;
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 1920, 1080), BeforeHudStatus::SplitViewport);
    sample.HasViewport = true;

    sample.HasTarget = false;
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 1920, 1080), BeforeHudStatus::TargetMismatch);

    sample.IsSameThread = false;
    EXPECT_EQ(RenderService::FindBeforeHudBlocker(sample, true, 1920, 1080), BeforeHudStatus::OtherThread);
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