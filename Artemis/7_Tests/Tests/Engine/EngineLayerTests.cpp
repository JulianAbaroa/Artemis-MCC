#include <gtest/gtest.h>
#include <d3d11.h>

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
import Engine.Base.Pov.Type;
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
    render.m_ChudDetour.Uninstall();
}

TEST(EngineLayerTests, FirstPersonDetourWithoutTheGameInstallsNothing)
{
    Services::Layer service{};
    Engine::Base::Layer base{ service };
    Engine::Render::Layer render{ service, base };

    // No haloreach.dll in the process: no function is found, nothing is hooked and the store says so.
    EXPECT_FALSE(render.m_FirstPersonDetour.Install());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonModelHooked());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonUnitHooked());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonDepthPassHooked());
    EXPECT_FALSE(render.m_RenderStore.IsOpaquePassHooked());
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
    store.EndFrame();
    EXPECT_TRUE(store.WasThirdPersonLastFrame());
    store.EndFrame();
    EXPECT_FALSE(store.WasThirdPersonLastFrame());

    // The same for the true answers of the first person test for user 0 (VW.6b.1).
    EXPECT_FALSE(store.WasFirstPersonUnitLastFrame());
    EXPECT_EQ(store.GetFirstPersonUnitObjectLastFrame(), 0xFFFFFFFFu);
    store.CountFirstPersonUnit(0x00050007);
    store.CountFirstPersonUnit(0x00050009);
    EXPECT_FALSE(store.WasFirstPersonUnitLastFrame());
    EXPECT_EQ(store.GetFirstPersonUnitObjectLastFrame(), 0xFFFFFFFFu);
    store.EndFrame();
    EXPECT_TRUE(store.WasFirstPersonUnitLastFrame());

    // The object of the frame is the first one the game answered for (VW.9a: the unit of the watched player).
    EXPECT_EQ(store.GetFirstPersonUnitObjectLastFrame(), 0x00050007u);
    store.EndFrame();
    EXPECT_FALSE(store.WasFirstPersonUnitLastFrame());
    EXPECT_EQ(store.GetFirstPersonUnitObjectLastFrame(), 0xFFFFFFFFu);
}

TEST(EngineLayerTests, MaskPlanCreatesReplacesAndReleasesTheTexturesOnlyWhenNeeded)
{
    using Engine::Render::System::RenderService;
    using Engine::Render::Type::MaskPlan;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = 1920;
    desc.Height = 1080;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
    desc.SampleDesc.Count = 1;

    D3D11_TEXTURE2D_DESC other{ desc };
    other.Width = 1280;

    const std::optional<D3D11_TEXTURE2D_DESC> wanted{ desc };
    const std::optional<D3D11_TEXTURE2D_DESC> none{};

    // Not requested: nothing to do, or release what was made.
    EXPECT_EQ(RenderService::PlanMask(false, wanted, false, {}), MaskPlan::Keep);
    EXPECT_EQ(RenderService::PlanMask(false, wanted, true, desc), MaskPlan::Release);

    // Requested but the resource is not known yet.
    EXPECT_EQ(RenderService::PlanMask(true, none, false, {}), MaskPlan::Keep);

    // First creation, a change of size, and nothing when the textures are those of the description.
    EXPECT_EQ(RenderService::PlanMask(true, wanted, false, {}), MaskPlan::Create);
    EXPECT_EQ(RenderService::PlanMask(true, wanted, true, other), MaskPlan::Create);
    EXPECT_EQ(RenderService::PlanMask(true, wanted, true, desc), MaskPlan::Keep);

    D3D11_TEXTURE2D_DESC otherFormat{ desc };
    otherFormat.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    EXPECT_EQ(RenderService::PlanMask(true, std::optional<D3D11_TEXTURE2D_DESC>{ otherFormat }, true, desc), MaskPlan::Create);
}

TEST(EngineLayerTests, ScaleToMaskIsTheIdentityForEqualSizesAndStaysInsideTheMask)
{
    using Engine::Render::System::RenderService;

    EXPECT_EQ(RenderService::ScaleToMask(0, 1920, 1920), 0u);
    EXPECT_EQ(RenderService::ScaleToMask(1234, 1920, 1920), 1234u);
    EXPECT_EQ(RenderService::ScaleToMask(1919, 1920, 1920), 1919u);

    // The mask is half the target: two pixels per texel.
    EXPECT_EQ(RenderService::ScaleToMask(0, 1920, 960), 0u);
    EXPECT_EQ(RenderService::ScaleToMask(1, 1920, 960), 0u);
    EXPECT_EQ(RenderService::ScaleToMask(1919, 1920, 960), 959u);

    // The mask is larger than the target.
    EXPECT_EQ(RenderService::ScaleToMask(10, 100, 200), 20u);
    EXPECT_EQ(RenderService::ScaleToMask(99, 100, 200), 198u);

    // A pixel past the target stays inside the mask, a size of 0 gives 0.
    EXPECT_EQ(RenderService::ScaleToMask(5000, 1920, 960), 959u);
    EXPECT_EQ(RenderService::ScaleToMask(5, 0, 960), 0u);
    EXPECT_EQ(RenderService::ScaleToMask(5, 1920, 0), 0u);
}

TEST(EngineLayerTests, RenderStoreMaskCopyIsTakenOncePerFrameAndPairAndReportedForTheLastFrame)
{
    using Engine::Render::Type::MaskPairKind;

    Engine::Render::State::RenderStore store{};

    EXPECT_FALSE(store.IsFirstPersonMaskRequested());
    store.SetFirstPersonMaskRequested(true);
    EXPECT_TRUE(store.IsFirstPersonMaskRequested());

    // Only the first caller of a frame takes the copy of a pair, and the pairs are independent.
    EXPECT_TRUE(store.TakeMaskCopy(MaskPairKind::DepthB));
    EXPECT_FALSE(store.TakeMaskCopy(MaskPairKind::DepthB));
    EXPECT_TRUE(store.TakeMaskCopy(MaskPairKind::DepthZero));
    EXPECT_FALSE(store.IsMaskCopied(MaskPairKind::DepthB));

    store.MarkMaskCopied(MaskPairKind::DepthZero);
    EXPECT_TRUE(store.IsMaskCopied(MaskPairKind::DepthZero));
    EXPECT_FALSE(store.IsMaskCopied(MaskPairKind::DepthB));
    EXPECT_FALSE(store.WasMaskCopiedLastFrame(MaskPairKind::DepthZero));

    // Closing the frame moves the marks to the last frame and frees the copies for the next one.
    store.EndFrame();
    EXPECT_FALSE(store.IsMaskCopied(MaskPairKind::DepthZero));
    EXPECT_TRUE(store.WasMaskCopiedLastFrame(MaskPairKind::DepthZero));
    EXPECT_FALSE(store.WasMaskCopiedLastFrame(MaskPairKind::DepthB));
    EXPECT_TRUE(store.TakeMaskCopy(MaskPairKind::DepthB));
    EXPECT_TRUE(store.TakeMaskCopy(MaskPairKind::DepthZero));

    // A frame with no copy leaves no mask for the one after.
    store.EndFrame();
    EXPECT_FALSE(store.WasMaskCopiedLastFrame(MaskPairKind::DepthZero));

    // Without a device nothing is ready, and releasing is harmless.
    EXPECT_FALSE(store.AreMaskTexturesReady(MaskPairKind::DepthB));
    EXPECT_FALSE(store.IsMaskAttempted(MaskPairKind::DepthZero));
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = 64;
    store.NoteMaskDesc(MaskPairKind::DepthZero, desc);
    ASSERT_TRUE(store.GetMaskWantedDesc(MaskPairKind::DepthZero).has_value());
    EXPECT_EQ(store.GetMaskWantedDesc(MaskPairKind::DepthZero)->Width, 64u);
    EXPECT_FALSE(store.GetMaskWantedDesc(MaskPairKind::DepthB).has_value());
    store.SetMaskTextures(MaskPairKind::DepthZero, {}, {}, {}, {}, desc);
    EXPECT_TRUE(store.IsMaskAttempted(MaskPairKind::DepthZero));
    EXPECT_FALSE(store.IsMaskAttempted(MaskPairKind::DepthB));
    EXPECT_FALSE(store.AreMaskTexturesReady(MaskPairKind::DepthZero));
    store.ReleaseMaskTextures();
    EXPECT_FALSE(store.IsMaskAttempted(MaskPairKind::DepthZero));
    EXPECT_FALSE(store.GetMaskWantedDesc(MaskPairKind::DepthZero).has_value());
}

TEST(EngineLayerTests, ZeroPassMaskSourceIsTheBoundDepthThenTheSavedOneThenNone)
{
    using Engine::Render::Type::ChooseZeroMaskSource;
    using Engine::Render::Type::MaskSource;

    EXPECT_EQ(ChooseZeroMaskSource(true, true), MaskSource::Bound);
    EXPECT_EQ(ChooseZeroMaskSource(true, false), MaskSource::Bound);
    EXPECT_EQ(ChooseZeroMaskSource(false, true), MaskSource::Saved);
    EXPECT_EQ(ChooseZeroMaskSource(false, false), MaskSource::None);
}

TEST(EngineLayerTests, MaskConstantsFlagOnlyTheUsablePairsAndTakeTheirSizes)
{
    using Engine::Render::Type::MakeMaskConstants;
    using Engine::Render::Type::MaskContext;
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::MaskPairView;

    // The views are never dereferenced: any non null pointer will do.
    auto* const view{ reinterpret_cast<ID3D11ShaderResourceView*>(static_cast<std::uintptr_t>(0x10)) };
    const auto pairIndex = [](MaskPairKind kind) { return static_cast<std::size_t>(kind); };

    // Nothing usable: no flags and the size 1 so a division never fails.
    MaskContext none{};
    const auto off{ MakeMaskConstants(none, 1920, 1080) };
    EXPECT_EQ(off.ValidFlags, 0u);
    EXPECT_EQ(off.DepthBWidth, 1u);
    EXPECT_EQ(off.TargetWidth, 1u);

    // Pair 0xB copied, the depth 0 pair not copied this frame.
    MaskContext mask{};
    mask.Pairs[pairIndex(MaskPairKind::DepthB)] = MaskPairView{ true, view, view, 1920, 1080 };
    mask.Pairs[pairIndex(MaskPairKind::DepthZero)] = MaskPairView{ false, view, view, 960, 540 };

    const auto constants{ MakeMaskConstants(mask, 1280, 720) };
    EXPECT_EQ(constants.ValidFlags, 0b01u);
    EXPECT_EQ(constants.DepthBWidth, 1920u);
    EXPECT_EQ(constants.DepthBHeight, 1080u);
    EXPECT_EQ(constants.DepthZeroWidth, 1u);
    EXPECT_EQ(constants.TargetWidth, 1280u);
    EXPECT_EQ(constants.TargetHeight, 720u);

    // A missing view, a size of 0 or a target of 0 leaves the pair out.
    mask.Pairs[pairIndex(MaskPairKind::DepthZero)] = MaskPairView{ true, view, nullptr, 960, 540 };
    EXPECT_EQ(MakeMaskConstants(mask, 1280, 720).ValidFlags, 0b01u);
    mask.Pairs[pairIndex(MaskPairKind::DepthZero)] = MaskPairView{ true, view, view, 960, 540 };
    EXPECT_EQ(MakeMaskConstants(mask, 1280, 720).ValidFlags, 0b11u);
    EXPECT_EQ(MakeMaskConstants(mask, 1280, 720).DepthZeroWidth, 960u);
    mask.Pairs[pairIndex(MaskPairKind::DepthZero)].Width = 0;
    EXPECT_EQ(MakeMaskConstants(mask, 1280, 720).ValidFlags, 0b01u);
    EXPECT_EQ(MakeMaskConstants(mask, 0, 720).ValidFlags, 0u);
}

TEST(EngineLayerTests, SceneColorScaleIsTheSliderAndAnInvalidSliderCountsAsOne)
{
    using Engine::Render::System::RenderService;

    constexpr float k_Infinity{ std::numeric_limits<float>::infinity() };

    EXPECT_FLOAT_EQ(RenderService::ComputeSceneColorScale(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(RenderService::ComputeSceneColorScale(2.0f), 2.0f);
    EXPECT_FLOAT_EQ(RenderService::ComputeSceneColorScale(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(RenderService::ComputeSceneColorScale(-1.0f), 1.0f);
    EXPECT_FLOAT_EQ(RenderService::ComputeSceneColorScale(k_Infinity), 1.0f);
    EXPECT_FLOAT_EQ(RenderService::ComputeSceneColorScale(std::numeric_limits<float>::quiet_NaN()), 1.0f);
}

TEST(EngineLayerTests, SceneColorStoreKeepsTheSliderAndTheScaleAndTheShaderGetsTheScale)
{
    using Engine::Render::State::RenderStore;
    using Engine::Render::Type::MakeMaskConstants;
    using Engine::Render::Type::MaskContext;

    RenderStore store{};

    // Before anything is set: the slider and the scale at 1.
    EXPECT_FLOAT_EQ(store.GetSceneColorSlider(), 1.0f);
    EXPECT_FLOAT_EQ(store.GetSceneColorScale(), 1.0f);

    store.SetSceneColorSlider(0.125f);
    EXPECT_FLOAT_EQ(store.GetSceneColorSlider(), 0.125f);

    store.SetSceneColorScale(0.5f);
    EXPECT_FLOAT_EQ(store.GetSceneColorScale(), 0.5f);

    // The shader constants carry the scale, with or without a mask, and 1 by default.
    EXPECT_FLOAT_EQ(MakeMaskConstants(MaskContext{}, 1920, 1080).ColorScale, 1.0f);
    EXPECT_FLOAT_EQ(MakeMaskConstants(MaskContext{}, 1920, 1080, 0.25f).ColorScale, 0.25f);
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

TEST(EngineLayerTests, PovServiceWithoutTheGameInstallsNothingAndDoesNothing)
{
    Services::Layer service{};
    Engine::Base::Layer base{ service };

    // No haloreach.dll in the process: the function is not found, the status says so, and Update and Restore do nothing.
    EXPECT_FALSE(base.m_PovService.Install());
    EXPECT_FALSE(base.m_PovStatusStore.Read().IsInstalled);

    base.m_PovRequestStore.Publish(0x00030001);
    base.m_PovService.Update(0);
    base.m_PovService.Update(1);
    base.m_PovService.Release(std::chrono::milliseconds{ 5 });
    base.m_PovService.Uninstall();

    const Engine::Base::Pov::Type::PovStatus status{ base.m_PovStatusStore.Read() };
    EXPECT_EQ(status.RequestedHandle, 0xFFFFFFFFu);
    EXPECT_EQ(status.Calls, 0u);
    EXPECT_EQ(status.Reasserts, 0u);
    EXPECT_EQ(status.Faults, 0u);
}

TEST(EngineLayerTests, PovProbeDetoursWithoutTheGameInstallNothing)
{
    Services::Layer service{};
    Engine::Base::Layer base{ service };

    // No haloreach.dll in the process: none of the eight signatures is found, nothing is hooked, and Uninstall of hooks that were never installed
    // is a no-op.
    EXPECT_FALSE(base.m_PovProbeDetours.Install());
    base.m_PovProbeDetours.Uninstall();
    EXPECT_FALSE(base.m_PovProbeStore.IsActive());

    // An update with a request but no game does not turn the probe on, and nothing is left in the ring.
    base.m_PovRequestStore.Publish(0x00030001);
    base.m_PovService.Update(0);
    EXPECT_FALSE(base.m_PovProbeStore.IsActive());

    // Without a thread context the unit of the players cannot be read either: the probe stays off with no request.
    base.m_PovRequestStore.Clear();
    base.m_PovService.Update(0);
    EXPECT_FALSE(base.m_PovProbeStore.IsActive());

    std::array<Engine::Base::Pov::Type::PovProbeEvent, 4> events{};
    EXPECT_EQ(base.m_PovProbeStore.Drain(events).Count, 0u);
}

TEST(EngineLayerTests, PovProbeFollowsTheScreensAndLetsThemGo)
{
    using Engine::Base::Pov::State::PovProbeStore;
    using Engine::Base::Pov::Type::PovProbeEvent;
    using Engine::Base::Pov::Type::PovProbeGoneScreen;
    using Engine::Base::Pov::Type::PovProbeKind;
    using Engine::Base::Pov::Type::PovProbeScreen;
    using Engine::Base::Pov::Type::PovProbeScreenField;

    PovProbeStore store{};
    store.SetTick(100);

    PovProbeScreen screen{};
    screen.Values[static_cast<std::size_t>(PovProbeScreenField::Flags)] = 0x88000;

    // A screen that shows up is new, with all its fields.
    store.NoteScreen(0x1000, screen, 1, false);
    std::array<PovProbeEvent, 8> events{};
    EXPECT_EQ(store.Drain(events).Count, 1u);
    EXPECT_EQ(events[0].Kind, PovProbeKind::ScreenNew);
    EXPECT_EQ(events[0].Args[0], 0x1000u);
    EXPECT_EQ(events[0].ExtraCount, screen.Values.size());
    EXPECT_EQ(events[0].Extra[static_cast<std::size_t>(PovProbeScreenField::Flags)], 0x88000u);

    // The same fields again note nothing, a changed one notes only its bit.
    store.NoteScreen(0x1000, screen, 1, false);
    EXPECT_EQ(store.Drain(events).Count, 0u);
    screen.Values[static_cast<std::size_t>(PovProbeScreenField::Flags)] = 0x89000;
    store.NoteScreen(0x1000, screen, 1, false);
    EXPECT_EQ(store.Drain(events).Count, 1u);
    EXPECT_EQ(events[0].Kind, PovProbeKind::ScreenChange);
    EXPECT_EQ(events[0].Args[1], 1ull << static_cast<std::size_t>(PovProbeScreenField::Flags));

    // A change seen when the update returned is its own kind, and a screen not noted at the start is not noted at the end.
    screen.Values[static_cast<std::size_t>(PovProbeScreenField::Flags)] = 0x8B000;
    store.NoteScreen(0x1000, screen, 1, true);
    EXPECT_EQ(store.Drain(events).Count, 1u);
    EXPECT_EQ(events[0].Kind, PovProbeKind::ScreenChangePost);
    store.NoteScreen(0x7000, screen, 1, true);
    EXPECT_EQ(store.Drain(events).Count, 0u);

    // Another vtable at a known address is a new screen, and so is the screen a creator returned at that address.
    PovProbeScreen reborn{ screen };
    reborn.Values[static_cast<std::size_t>(PovProbeScreenField::Vtable)] = 0x55;
    store.NoteScreen(0x1000, reborn, 1, false);
    EXPECT_EQ(store.Drain(events).Count, 1u);
    EXPECT_EQ(events[0].Kind, PovProbeKind::ScreenNew);
    store.ForgetScreen(0x1000);
    store.NoteScreen(0x1000, reborn, 1, false);
    EXPECT_EQ(store.Drain(events).Count, 1u);
    EXPECT_EQ(events[0].Kind, PovProbeKind::ScreenNew);
    screen = reborn;
    screen.Values[static_cast<std::size_t>(PovProbeScreenField::Flags)] = 0x8B000;
    store.NoteScreen(0x1000, screen, 1, true);
    store.Drain(events);

    // The screen is gone only after the age limit, with the calls it got.
    std::array<PovProbeGoneScreen, 4> gone{};
    EXPECT_EQ(store.ExpireScreens(159, gone), 0u);
    EXPECT_EQ(store.ExpireScreens(160, gone), 1u);
    EXPECT_EQ(gone[0].This, 0x1000u);
    EXPECT_EQ(gone[0].Calls, 1u);
    EXPECT_EQ(store.ExpireScreens(400, gone), 0u);

    // The table holds sixteen screens: the seventeenth takes the place of the one called least recently, and says so.
    store.SetTick(200);
    for (std::uint64_t index{ 0 }; index < 16; ++index) store.NoteScreen(0x2000 + index, screen, 1, false);
    std::array<PovProbeEvent, 32> many{};
    EXPECT_EQ(store.Drain(many).Count, 16u);
    store.SetTick(201);
    store.NoteScreen(0x2000, screen, 1, false);
    store.Drain(many);
    store.NoteScreen(0x3000, screen, 1, false);
    const std::size_t count{ store.Drain(many).Count };
    ASSERT_EQ(count, 2u);
    EXPECT_EQ(many[0].Kind, PovProbeKind::ScreenEvict);
    EXPECT_EQ(many[0].Args[0], 0x2001u);
    EXPECT_EQ(many[0].Args[1], 1u);
    EXPECT_EQ(many[1].Kind, PovProbeKind::ScreenNew);

    std::array<PovProbeGoneScreen, 32> all{};
    EXPECT_EQ(store.ExpireScreens(1000, all), 16u);
}

TEST(EngineLayerTests, PovProbeKeepsTheCreationsInTheirOwnRing)
{
    using Engine::Base::Pov::State::PovProbeStore;
    using Engine::Base::Pov::Type::PovProbeEvent;
    using Engine::Base::Pov::Type::PovProbeKind;

    PovProbeStore store{};

    // More creations than the ring holds do not touch the calls, and the loss is counted on the creation ring only.
    PovProbeEvent call{};
    call.Kind = PovProbeKind::ViewUnit;
    store.Record(call);

    PovProbeEvent create{};
    create.Kind = PovProbeKind::ScreenCreate;
    create.ExtraCount = 2;
    create.Extra[1] = 0x42;
    for (std::uint64_t index{ 0 }; index < 70; ++index)
    {
        create.Result = index;
        store.Record(create);
    }

    std::array<PovProbeEvent, 128> events{};
    const auto calls{ store.Drain(events) };
    EXPECT_EQ(calls.Count, 1u);
    EXPECT_EQ(calls.Dropped, 0u);
    EXPECT_EQ(events[0].Kind, PovProbeKind::ViewUnit);

    const auto creations{ store.DrainCreations(events) };
    EXPECT_EQ(creations.Count, 64u);
    EXPECT_EQ(creations.Dropped, 6u);
    EXPECT_EQ(events[0].Result, 6u);
    EXPECT_EQ(events[0].Extra[1], 0x42u);
}

TEST(EngineLayerTests, SceneBlockerTakesTheHdrTargetOfTheGame)
{
    using Engine::Render::System::RenderService;
    using Engine::Render::Type::SceneStatus;
    using Engine::Render::Type::SceneTargetSample;

    SceneTargetSample sample{};
    sample.IsSameThread = true;
    sample.HasTarget = true;
    sample.IsSameDevice = true;
    sample.HasViewport = true;
    sample.Width = 1920;
    sample.Height = 1080;
    sample.SampleCount = 1;
    sample.ViewportWidth = 1920.0f;
    sample.ViewportHeight = 1080.0f;

    // The valid case: the target of the game is the HDR target, one sample and the size of the back buffer.
    EXPECT_FALSE(RenderService::FindSceneBlocker(sample, true, 1920, 1080).has_value());

    EXPECT_EQ(RenderService::FindSceneBlocker(sample, false, 1920, 1080), SceneStatus::NotReady);
    EXPECT_EQ(RenderService::FindSceneBlocker(sample, true, 1280, 720), SceneStatus::TargetMismatch);
    EXPECT_EQ(RenderService::FindSceneBlocker(sample, true, 0, 0), SceneStatus::TargetMismatch);

    sample.SampleCount = 4;
    EXPECT_EQ(RenderService::FindSceneBlocker(sample, true, 1920, 1080), SceneStatus::Multisampled);
    sample.SampleCount = 1;

    sample.IsSameDevice = false;
    EXPECT_EQ(RenderService::FindSceneBlocker(sample, true, 1920, 1080), SceneStatus::OtherDevice);
    sample.IsSameDevice = true;

    sample.ViewportWidth = 960.0f;
    EXPECT_EQ(RenderService::FindSceneBlocker(sample, true, 1920, 1080), SceneStatus::SplitViewport);
    sample.ViewportWidth = 1920.0f;
    sample.HasViewport = false;
    EXPECT_EQ(RenderService::FindSceneBlocker(sample, true, 1920, 1080), SceneStatus::SplitViewport);
    sample.HasViewport = true;

    sample.HasTarget = false;
    EXPECT_EQ(RenderService::FindSceneBlocker(sample, true, 1920, 1080), SceneStatus::TargetMismatch);
    sample.HasTarget = true;

    sample.IsSameThread = false;
    EXPECT_EQ(RenderService::FindSceneBlocker(sample, true, 1920, 1080), SceneStatus::OtherThread);
}

TEST(EngineLayerTests, SceneDrawPlaceAndStatusAreOfTheLastCompletedFrame)
{
    using Engine::Render::State::RenderStore;
    using Engine::Render::Type::SceneStatus;
    using Engine::Render::Type::DrawPlace;

    RenderStore store{};

    // Only the first call of the Present frame takes the scene point.
    EXPECT_TRUE(store.TakeSceneCall());
    EXPECT_FALSE(store.TakeSceneCall());

    store.SetSceneStatus(SceneStatus::Drawn);
    store.SetDrawPlace(DrawPlace::Scene);
    EXPECT_EQ(store.GetDrawPlace(), DrawPlace::None);
    EXPECT_EQ(store.GetSceneStatus(), SceneStatus::NoSceneCall);

    store.NextPresentFrame();
    EXPECT_EQ(store.GetDrawPlace(), DrawPlace::Scene);
    EXPECT_EQ(store.GetSceneStatus(), SceneStatus::Drawn);
    EXPECT_TRUE(store.TakeSceneCall());

    // The next frame starts with no scene call and no place, and a blocked frame keeps its reason.
    store.SetSceneStatus(SceneStatus::SplitViewport);
    store.NextPresentFrame();
    EXPECT_EQ(store.GetDrawPlace(), DrawPlace::None);
    EXPECT_EQ(store.GetSceneStatus(), SceneStatus::SplitViewport);
    store.NextPresentFrame();
    EXPECT_EQ(store.GetDrawPlace(), DrawPlace::None);
    EXPECT_EQ(store.GetSceneStatus(), SceneStatus::NoSceneCall);

    EXPECT_EQ(store.GetSceneFailures(), 0u);
    store.CountSceneFailure();
    EXPECT_EQ(store.GetSceneFailures(), 1u);
}

TEST(EngineLayerTests, SceneDrawRequestIsOffByDefaultAndFollowsTheViewer)
{
    using Engine::Render::State::RenderStore;

    RenderStore store{};
    EXPECT_FALSE(store.IsSceneDrawRequested());
    store.SetSceneDrawRequested(true);
    EXPECT_TRUE(store.IsSceneDrawRequested());
    store.SetSceneDrawRequested(false);
    EXPECT_FALSE(store.IsSceneDrawRequested());
}

TEST(EngineLayerTests, RenderStoreHudFlag)
{
    Engine::Render::State::RenderStore store{};

    EXPECT_FALSE(store.IsGameHudHidden());
    store.SetGameHudHidden(true);
    EXPECT_TRUE(store.IsGameHudHidden());
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
