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

    // No haloreach.dll in the process: no function is found, nothing is hooked and the store says so.
    EXPECT_FALSE(render.m_FirstPersonDetour.Install());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonModelHooked());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonUnitHooked());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonDepthPassHooked());
    EXPECT_FALSE(render.m_RenderStore.IsFirstPersonForwardHooked());
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

    // The same for the true answers of the first person test for user 0 (VW.6b.1).
    EXPECT_FALSE(store.WasFirstPersonUnitLastFrame());
    store.CountFirstPersonUnit();
    EXPECT_FALSE(store.WasFirstPersonUnitLastFrame());
    store.EndHudFrame();
    EXPECT_TRUE(store.WasFirstPersonUnitLastFrame());
    store.EndHudFrame();
    EXPECT_FALSE(store.WasFirstPersonUnitLastFrame());
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

TEST(EngineLayerTests, ReduceStencilRespectsRowPitchAndRegions)
{
    using Engine::Render::System::RenderService;
    using Engine::Render::Type::StencilProbeInfo;
    using Engine::Render::Type::StencilReadStatus;
    using Engine::Render::Type::StencilRegion;

    // A texture of 8x8 texels of 8 bytes with a row pitch of 80 bytes (16 padding bytes of 0xFF per row, they must not be read).
    constexpr std::uint32_t width{ 8 };
    constexpr std::uint32_t height{ 8 };
    constexpr std::uint32_t pitch{ 80 };
    std::vector<std::uint8_t> data(static_cast<std::size_t>(pitch) * height, 0xFF);

    const auto put = [&data](std::uint32_t x, std::uint32_t y, float depth, std::uint8_t stencil)
    {
        std::uint8_t* texel{ data.data() + static_cast<std::size_t>(y) * pitch + static_cast<std::size_t>(x) * 8 };
        std::memcpy(texel, &depth, sizeof(depth));
        texel[4] = stencil;
        texel[5] = texel[6] = texel[7] = 0;
    };

    for (std::uint32_t y{ 0 }; y < height; ++y)
    {
        for (std::uint32_t x{ 0 }; x < width; ++x)
        {
            put(x, y, x >= 4 && y >= 4 ? 1.0f : 0.5f, x >= 4 && y >= 4 ? 0xA4 : 0x00);
        }
    }

    put(3, 3, 0.5f, 0x01);      // the one texel of the center box
    put(0, 0, 0.0f, 0x80);      // top, depth 0
    put(0, 1, 0.5f, 0x80);      // top

    const StencilProbeInfo info{ RenderService::ReduceStencil(data.data(), pitch, width, height) };
    ASSERT_EQ(info.Status, StencilReadStatus::Read);

    const auto& all{ info.Regions[static_cast<std::size_t>(StencilRegion::All)] };
    EXPECT_EQ(all.Texels, 64u);
    EXPECT_EQ(all.Histogram[0x00], 45u);
    EXPECT_EQ(all.Histogram[0xA4], 16u);
    EXPECT_EQ(all.Histogram[0x01], 1u);
    EXPECT_EQ(all.Histogram[0x80], 2u);
    EXPECT_EQ(all.BitCounts[0], 1u);
    EXPECT_EQ(all.BitCounts[2], 16u);
    EXPECT_EQ(all.BitCounts[5], 16u);
    EXPECT_EQ(all.BitCounts[7], 18u);
    EXPECT_EQ(all.BitCounts[1], 0u);
    EXPECT_FLOAT_EQ(all.MinDepth, 0.0f);
    EXPECT_FLOAT_EQ(all.MaxDepth, 1.0f);
    EXPECT_EQ(all.ZeroDepthTexels, 1u);

    const auto& weapon{ info.Regions[static_cast<std::size_t>(StencilRegion::Weapon)] };
    EXPECT_EQ(weapon.Texels, 16u);
    EXPECT_EQ(weapon.Histogram[0xA4], 16u);
    EXPECT_FLOAT_EQ(weapon.MinDepth, 1.0f);
    EXPECT_FLOAT_EQ(weapon.MaxDepth, 1.0f);
    EXPECT_EQ(weapon.ZeroDepthTexels, 0u);

    const auto& center{ info.Regions[static_cast<std::size_t>(StencilRegion::Center)] };
    EXPECT_EQ(center.Texels, 1u);
    EXPECT_EQ(center.Histogram[0x01], 1u);
    EXPECT_FLOAT_EQ(center.MinDepth, 0.5f);

    const auto& top{ info.Regions[static_cast<std::size_t>(StencilRegion::Top)] };
    EXPECT_EQ(top.Texels, 16u);
    EXPECT_EQ(top.Histogram[0x80], 2u);
    EXPECT_EQ(top.Histogram[0x00], 14u);
    EXPECT_FLOAT_EQ(top.MinDepth, 0.0f);
    EXPECT_FLOAT_EQ(top.MaxDepth, 0.5f);
    EXPECT_EQ(top.ZeroDepthTexels, 1u);

    // The line of a region lists the most frequent values first.
    const std::string line{ RenderService::DescribeStencilRegion(StencilRegion::Weapon, weapon) };
    EXPECT_NE(line.find("stencil weapon n=16 top 0XA4:16 bits"), std::string::npos) << line;
    EXPECT_NE(line.find("b2:16"), std::string::npos) << line;
    EXPECT_NE(line.find("zero=0"), std::string::npos) << line;

    const std::string allLine{ RenderService::DescribeStencilRegion(StencilRegion::All, all) };
    EXPECT_NE(allLine.find("top 0X00:45 0XA4:16 0X80:2 0X01:1 bits"), std::string::npos) << allLine;

    // A pitch smaller than a row or no data is refused.
    EXPECT_EQ(RenderService::ReduceStencil(data.data(), 32, width, height).Status, StencilReadStatus::BadLayout);
    EXPECT_EQ(RenderService::ReduceStencil(nullptr, pitch, width, height).Status, StencilReadStatus::BadLayout);
}

TEST(EngineLayerTests, CompareDepthRespectsRowPitchBoxAndRange)
{
    using Engine::Render::System::RenderService;

    // Two textures of 8x8 texels of 8 bytes, row pitch 80 for the first and 96 for the second (padding 0xFF, must not be read).
    constexpr std::uint32_t width{ 8 };
    constexpr std::uint32_t height{ 8 };
    constexpr std::uint32_t pitchA{ 80 };
    constexpr std::uint32_t pitchB{ 96 };
    std::vector<std::uint8_t> a(static_cast<std::size_t>(pitchA) * height, 0xFF);
    std::vector<std::uint8_t> b(static_cast<std::size_t>(pitchB) * height, 0xFF);

    const auto put = [](std::vector<std::uint8_t>& data, std::uint32_t pitch, std::uint32_t x, std::uint32_t y, float depth, std::uint8_t stencil)
    {
        std::uint8_t* texel{ data.data() + static_cast<std::size_t>(y) * pitch + static_cast<std::size_t>(x) * 8 };
        std::memcpy(texel, &depth, sizeof(depth));
        texel[4] = stencil;
        texel[5] = texel[6] = texel[7] = 0;
    };

    for (std::uint32_t y{ 0 }; y < height; ++y)
    {
        for (std::uint32_t x{ 0 }; x < width; ++x)
        {
            put(a, pitchA, x, y, 0.25f, 0);
            put(b, pitchB, x, y, 0.25f, 0);
        }
    }

    // Nothing changed.
    const auto same{ RenderService::CompareDepth(a.data(), pitchA, b.data(), pitchB, width, height) };
    ASSERT_TRUE(same.has_value());
    EXPECT_EQ(same->Changed, 0u);
    EXPECT_EQ(same->Weapon, 0u);
    EXPECT_EQ(same->Right, 0u);

    // A change of the stencil byte only is not a change of depth.
    put(b, pitchB, 1, 1, 0.25f, 0x80);
    EXPECT_EQ(RenderService::CompareDepth(a.data(), pitchA, b.data(), pitchB, width, height)->Changed, 0u);

    // One texel outside the weapon region (the lower right quadrant).
    put(b, pitchB, 2, 3, 0.5f, 0);
    const auto one{ RenderService::CompareDepth(a.data(), pitchA, b.data(), pitchB, width, height) };
    ASSERT_TRUE(one.has_value());
    EXPECT_EQ(one->Changed, 1u);
    EXPECT_EQ(one->X0, 2u);
    EXPECT_EQ(one->Y0, 3u);
    EXPECT_EQ(one->X1, 2u);
    EXPECT_EQ(one->Y1, 3u);
    EXPECT_FLOAT_EQ(one->MinDepth, 0.5f);
    EXPECT_FLOAT_EQ(one->MaxDepth, 0.5f);
    EXPECT_EQ(one->Weapon, 0u);
    EXPECT_EQ(one->Right, 0u);

    // A box in the lower right: x 4..6, y 5..7 (9 texels), depths 0.75 and 1.0, and the first one that is outside it.
    for (std::uint32_t y{ 5 }; y <= 7; ++y)
    {
        for (std::uint32_t x{ 4 }; x <= 6; ++x) put(b, pitchB, x, y, 0.75f, 0);
    }

    put(b, pitchB, 6, 7, 1.0f, 0);

    const auto box{ RenderService::CompareDepth(a.data(), pitchA, b.data(), pitchB, width, height) };
    ASSERT_TRUE(box.has_value());
    EXPECT_EQ(box->Changed, 10u);
    EXPECT_EQ(box->X0, 2u);
    EXPECT_EQ(box->Y0, 3u);
    EXPECT_EQ(box->X1, 6u);
    EXPECT_EQ(box->Y1, 7u);
    EXPECT_FLOAT_EQ(box->MinDepth, 0.5f);
    EXPECT_FLOAT_EQ(box->MaxDepth, 1.0f);
    EXPECT_EQ(box->Weapon, 9u);

    // Right half is x >= 4: the box (x 4..6) is all in it, the first texel (x 2) is not.
    EXPECT_EQ(box->Right, 9u);

    // A layout that cannot be read.
    EXPECT_FALSE(RenderService::CompareDepth(a.data(), 32, b.data(), pitchB, width, height).has_value());
    EXPECT_FALSE(RenderService::CompareDepth(a.data(), pitchA, b.data(), 32, width, height).has_value());
    EXPECT_FALSE(RenderService::CompareDepth(nullptr, pitchA, b.data(), pitchB, width, height).has_value());
    EXPECT_FALSE(RenderService::CompareDepth(a.data(), pitchA, b.data(), pitchB, 0, height).has_value());
}

TEST(EngineLayerTests, CompareTexelsComparesAColorOf4BytesPerTexelWithPaddedRows)
{
    using Engine::Render::System::RenderService;
    using Engine::Render::Type::ChangedMap;
    using Engine::Render::Type::k_ColorTexelLayout;

    constexpr std::uint32_t width{ 8 };
    constexpr std::uint32_t height{ 4 };
    constexpr std::uint32_t pitchA{ 48 };
    constexpr std::uint32_t pitchB{ 40 };
    std::vector<std::uint8_t> a(static_cast<std::size_t>(pitchA) * height, 0x11);
    std::vector<std::uint8_t> b(static_cast<std::size_t>(pitchB) * height, 0x11);

    ChangedMap map{};
    const auto same{ RenderService::CompareTexels(a.data(), pitchA, b.data(), pitchB, width, height, k_ColorTexelLayout, &map) };
    ASSERT_TRUE(same.has_value());
    EXPECT_EQ(same->Changed, 0u);
    EXPECT_EQ(map.Width, width);
    EXPECT_EQ(map.Height, height);
    ASSERT_EQ(map.Texels.size(), static_cast<std::size_t>(width) * height);
    EXPECT_EQ(std::count(map.Texels.begin(), map.Texels.end(), std::uint8_t{ 1 }), 0);

    // The last byte of a texel counts, and the padding of the rows (bytes past width * 4) does not.
    b[1 * pitchB + 3 * 4 + 3] = 0x22;
    b[2 * pitchB + 7 * 4] = 0x22;
    b[0 * pitchB + 33] = 0x22;
    b[3 * pitchB + 39] = 0x22;

    const auto diff{ RenderService::CompareTexels(a.data(), pitchA, b.data(), pitchB, width, height, k_ColorTexelLayout, &map) };
    ASSERT_TRUE(diff.has_value());
    EXPECT_EQ(diff->Changed, 2u);
    EXPECT_EQ(diff->X0, 3u);
    EXPECT_EQ(diff->Y0, 1u);
    EXPECT_EQ(diff->X1, 7u);
    EXPECT_EQ(diff->Y1, 2u);
    EXPECT_FLOAT_EQ(diff->MinDepth, 0.0f);
    EXPECT_FLOAT_EQ(diff->MaxDepth, 0.0f);
    EXPECT_EQ(diff->Right, 1u);
    EXPECT_EQ(diff->Weapon, 1u);
    EXPECT_EQ(map.Texels[1 * width + 3], 1);
    EXPECT_EQ(map.Texels[2 * width + 7], 1);
    EXPECT_EQ(std::count(map.Texels.begin(), map.Texels.end(), std::uint8_t{ 1 }), 2);

    // The pitch is smaller than a row of 4 bytes per texel.
    EXPECT_FALSE(RenderService::CompareTexels(a.data(), 24, b.data(), pitchB, width, height, k_ColorTexelLayout, nullptr).has_value());

    // A layout that is not valid.
    EXPECT_FALSE(RenderService::CompareTexels(a.data(), pitchA, b.data(), pitchB, width, height, { .TexelBytes = 4, .CompareBytes = 5 }, nullptr)
        .has_value());
}

TEST(EngineLayerTests, CountOutsideMaskCountsTheChangedColorTexelsThatTheDepthDidNotChange)
{
    using Engine::Render::System::RenderService;
    using Engine::Render::Type::ChangedMap;

    const auto make = [](std::uint32_t width, std::uint32_t height, std::initializer_list<std::size_t> ones)
    {
        ChangedMap map{ .Width = width, .Height = height, .Texels = std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height, 0) };
        for (const std::size_t i : ones) map.Texels[i] = 1;
        return map;
    };

    // All the color texels are inside the depth mask.
    EXPECT_EQ(RenderService::CountOutsideMask(make(4, 2, { 0, 1, 2, 5 }), make(4, 2, { 1, 2 })), 0u);

    // All outside.
    EXPECT_EQ(RenderService::CountOutsideMask(make(4, 2, { 0 }), make(4, 2, { 3, 4, 7 })), 3u);

    // Half: two of four are in the mask.
    EXPECT_EQ(RenderService::CountOutsideMask(make(4, 2, { 1, 2 }), make(4, 2, { 1, 2, 5, 6 })), 2u);

    // No color change at all, and a depth mask that changed nothing.
    EXPECT_EQ(RenderService::CountOutsideMask(make(4, 2, {}), make(4, 2, {})), 0u);
    EXPECT_EQ(RenderService::CountOutsideMask(make(4, 2, {}), make(4, 2, { 0, 1 })), 2u);

    // Sizes that differ, or a map that was not made: no count.
    EXPECT_FALSE(RenderService::CountOutsideMask(make(4, 2, { 0 }), make(2, 4, { 0 })).has_value());
    EXPECT_FALSE(RenderService::CountOutsideMask(make(8, 2, { 0 }), make(4, 2, { 0 })).has_value());
    EXPECT_FALSE(RenderService::CountOutsideMask(ChangedMap{}, make(4, 2, { 0 })).has_value());
    EXPECT_FALSE(RenderService::CountOutsideMask(make(4, 2, { 0 }), ChangedMap{}).has_value());
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
    EXPECT_FALSE(store.IsMaskCopied(MaskPairKind::Color));
    EXPECT_FALSE(store.WasMaskCopiedLastFrame(MaskPairKind::DepthZero));

    // Closing the frame moves the marks to the last frame and frees the copies for the next one.
    store.EndHudFrame();
    EXPECT_FALSE(store.IsMaskCopied(MaskPairKind::DepthZero));
    EXPECT_TRUE(store.WasMaskCopiedLastFrame(MaskPairKind::DepthZero));
    EXPECT_FALSE(store.WasMaskCopiedLastFrame(MaskPairKind::DepthB));
    EXPECT_TRUE(store.TakeMaskCopy(MaskPairKind::DepthB));
    EXPECT_TRUE(store.TakeMaskCopy(MaskPairKind::Color));

    // A frame with no copy leaves no mask for the one after.
    store.EndHudFrame();
    EXPECT_FALSE(store.WasMaskCopiedLastFrame(MaskPairKind::DepthZero));

    // Without a device nothing is ready, and releasing is harmless.
    EXPECT_FALSE(store.AreMaskTexturesReady(MaskPairKind::DepthB));
    EXPECT_FALSE(store.IsMaskAttempted(MaskPairKind::Color));
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = 64;
    store.NoteMaskDesc(MaskPairKind::Color, desc);
    ASSERT_TRUE(store.GetMaskWantedDesc(MaskPairKind::Color).has_value());
    EXPECT_EQ(store.GetMaskWantedDesc(MaskPairKind::Color)->Width, 64u);
    EXPECT_FALSE(store.GetMaskWantedDesc(MaskPairKind::DepthB).has_value());
    store.SetMaskTextures(MaskPairKind::Color, {}, {}, {}, {}, desc);
    EXPECT_TRUE(store.IsMaskAttempted(MaskPairKind::Color));
    EXPECT_FALSE(store.IsMaskAttempted(MaskPairKind::DepthB));
    EXPECT_FALSE(store.AreMaskTexturesReady(MaskPairKind::Color));
    store.ReleaseMaskTextures();
    EXPECT_FALSE(store.IsMaskAttempted(MaskPairKind::Color));
    EXPECT_FALSE(store.GetMaskWantedDesc(MaskPairKind::Color).has_value());
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

TEST(EngineLayerTests, ColorMaskPlanUsesTheDescriptionOfTheRenderTargetAndOnlySupportedFormatsGetAView)
{
    using Engine::Render::System::RenderService;
    using Engine::Render::Type::ColorMaskViewFormat;
    using Engine::Render::Type::MaskPlan;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = 1920;
    desc.Height = 1080;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    desc.SampleDesc.Count = 1;

    D3D11_TEXTURE2D_DESC other{ desc };
    other.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

    const std::optional<D3D11_TEXTURE2D_DESC> wanted{ desc };

    // Created from the noted description once, replaced when the format changes, kept for the same one, released with the option.
    EXPECT_EQ(RenderService::PlanMask(true, wanted, false, {}), MaskPlan::Create);
    EXPECT_EQ(RenderService::PlanMask(true, wanted, true, desc), MaskPlan::Keep);
    EXPECT_EQ(RenderService::PlanMask(true, wanted, true, other), MaskPlan::Create);
    EXPECT_EQ(RenderService::PlanMask(false, wanted, true, desc), MaskPlan::Release);
    EXPECT_EQ(RenderService::PlanMask(true, std::optional<D3D11_TEXTURE2D_DESC>{}, false, {}), MaskPlan::Keep);

    // The view has the format of the texture, a typeless 8 bit one is viewed as UNORM, anything else is not compared.
    EXPECT_EQ(ColorMaskViewFormat(DXGI_FORMAT_R11G11B10_FLOAT), DXGI_FORMAT_R11G11B10_FLOAT);
    EXPECT_EQ(ColorMaskViewFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    EXPECT_EQ(ColorMaskViewFormat(DXGI_FORMAT_R8G8B8A8_TYPELESS), DXGI_FORMAT_R8G8B8A8_UNORM);
    EXPECT_FALSE(ColorMaskViewFormat(DXGI_FORMAT_R16G16B16A16_FLOAT).has_value());
    EXPECT_FALSE(ColorMaskViewFormat(DXGI_FORMAT_R32G8X24_TYPELESS).has_value());
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

    // Nothing usable: no flags, the size 1 so a division never fails, and the debug paint stays off.
    MaskContext none{};
    const auto off{ MakeMaskConstants(none, true, 1920, 1080) };
    EXPECT_EQ(off.ValidFlags, 0u);
    EXPECT_EQ(off.MaskShown, 0u);
    EXPECT_EQ(off.DepthBWidth, 1u);
    EXPECT_EQ(off.TargetWidth, 1u);

    // Pair 0 and the color pair, the depth 0 pair not copied this frame.
    MaskContext mask{};
    mask.Pairs[pairIndex(MaskPairKind::DepthB)] = MaskPairView{ true, view, view, 1920, 1080 };
    mask.Pairs[pairIndex(MaskPairKind::DepthZero)] = MaskPairView{ false, view, view, 1920, 1080 };
    mask.Pairs[pairIndex(MaskPairKind::Color)] = MaskPairView{ true, view, view, 960, 540 };

    const auto constants{ MakeMaskConstants(mask, true, 1280, 720) };
    EXPECT_EQ(constants.ValidFlags, 0b101u);
    EXPECT_EQ(constants.MaskShown, 1u);
    EXPECT_EQ(constants.DepthBWidth, 1920u);
    EXPECT_EQ(constants.DepthBHeight, 1080u);
    EXPECT_EQ(constants.DepthZeroWidth, 1u);
    EXPECT_EQ(constants.ColorWidth, 960u);
    EXPECT_EQ(constants.ColorHeight, 540u);
    EXPECT_EQ(constants.TargetWidth, 1280u);
    EXPECT_EQ(constants.TargetHeight, 720u);
    EXPECT_EQ(MakeMaskConstants(mask, false, 1280, 720).MaskShown, 0u);

    // A missing view, a size of 0 or a target of 0 leaves the pair out.
    mask.Pairs[pairIndex(MaskPairKind::DepthZero)] = MaskPairView{ true, view, nullptr, 1920, 1080 };
    EXPECT_EQ(MakeMaskConstants(mask, false, 1280, 720).ValidFlags, 0b101u);
    mask.Pairs[pairIndex(MaskPairKind::DepthZero)] = MaskPairView{ true, view, view, 1920, 1080 };
    EXPECT_EQ(MakeMaskConstants(mask, false, 1280, 720).ValidFlags, 0b111u);
    mask.Pairs[pairIndex(MaskPairKind::Color)].Width = 0;
    EXPECT_EQ(MakeMaskConstants(mask, false, 1280, 720).ValidFlags, 0b011u);
    EXPECT_EQ(MakeMaskConstants(mask, false, 0, 720).ValidFlags, 0u);
}

TEST(EngineLayerTests, UnionChangedMapMergesTheTexelsOfTheSameSizeAndKeepsTheTargetOtherwise)
{
    using Engine::Render::Type::ChangedMap;
    using Engine::Render::Type::UnionChangedMap;

    const auto make = [](std::uint32_t width, std::uint32_t height, std::initializer_list<std::size_t> changed)
    {
        ChangedMap map{ .Width = width, .Height = height, .Texels = std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height, 0) };
        for (const std::size_t index : changed) map.Texels[index] = 1;
        return map;
    };

    ChangedMap target{};
    EXPECT_TRUE(UnionChangedMap(target, ChangedMap{}));
    EXPECT_TRUE(target.Texels.empty());

    // An empty target takes the first map, then the union adds the texels of the second.
    EXPECT_TRUE(UnionChangedMap(target, make(4, 2, { 1, 2 })));
    EXPECT_TRUE(UnionChangedMap(target, make(4, 2, { 2, 5 })));
    EXPECT_EQ(target.Texels, make(4, 2, { 1, 2, 5 }).Texels);

    // An empty source changes nothing, a map of another size is refused.
    EXPECT_TRUE(UnionChangedMap(target, ChangedMap{}));
    EXPECT_FALSE(UnionChangedMap(target, make(2, 4, { 0 })));
    EXPECT_EQ(target.Texels, make(4, 2, { 1, 2, 5 }).Texels);
}

TEST(EngineLayerTests, RenderStoreDepthPairNotesAreKeptInCallOrderUpToTheLimit)
{
    using Engine::Render::Type::DepthPairNote;
    using Engine::Render::Type::PairState;
    using Engine::Render::Type::k_DepthPairCount;

    Engine::Render::State::RenderStore store{};
    EXPECT_FALSE(store.GetDepthPairs().AreReady());
    EXPECT_EQ(store.GetDepthPairs().GetCopy(0), nullptr);
    EXPECT_EQ(store.GetDepthPairs().GetNoteCount(), 0u);

    for (std::size_t i{ 0 }; i < k_DepthPairCount; ++i)
    {
        const auto index{ store.GetDepthPairs().NoteCall({ .State = i == 1 ? PairState::Mismatch : PairState::Copied,
            .Caller = i == 2 ? Engine::Render::Type::FirstPersonCaller::Pass1 : Engine::Render::Type::FirstPersonCaller::Func26E2A0,
            .Pass = i == 2 ? 0xB : 0, .IsCaptured = i == 2 }, nullptr) };
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, i);
    }

    EXPECT_FALSE(store.GetDepthPairs().NoteCall({ .State = PairState::Copied }, nullptr).has_value());
    EXPECT_EQ(store.GetDepthPairs().GetNoteCount(), static_cast<std::uint32_t>(k_DepthPairCount));
    EXPECT_EQ(store.GetDepthPairs().GetNote(1).State, PairState::Mismatch);
    EXPECT_TRUE(store.GetDepthPairs().GetNote(2).IsCaptured);
    EXPECT_EQ(store.GetDepthPairs().GetNote(2).Caller, Engine::Render::Type::FirstPersonCaller::Pass1);
    EXPECT_EQ(store.GetDepthPairs().GetNote(2).Pass, 0xB);
    EXPECT_EQ(store.GetDepthPairs().GetNote(3).Caller, Engine::Render::Type::FirstPersonCaller::Func26E2A0);
    EXPECT_FALSE(store.GetDepthPairs().GetNote(3).IsCaptured);

    store.GetDepthPairs().Release();
    EXPECT_EQ(store.GetDepthPairs().GetNoteCount(), 0u);
    EXPECT_EQ(store.GetDepthPairs().GetNote(0).State, PairState::Free);
    EXPECT_FALSE(store.GetDepthPairs().GetSetup().IsAttempted);

    // Arming a probe also drops the notes of the old one.
    EXPECT_TRUE(store.GetDepthPairs().NoteCall({ .State = PairState::Copied }, nullptr).has_value());
    store.RequestFirstPersonProbe();
    EXPECT_FALSE(store.StepFirstPersonProbe());
    EXPECT_EQ(store.GetDepthPairs().GetNoteCount(), 0u);
}

TEST(EngineLayerTests, RenderStoreColorPairsKeepTheirNotesAndTheTargetDescription)
{
    using Engine::Render::Type::ColorPairNote;
    using Engine::Render::Type::PairState;
    using Engine::Render::Type::k_ColorPairCount;

    Engine::Render::State::RenderStore store{};
    EXPECT_FALSE(store.GetColorPairs().AreReady());
    EXPECT_FALSE(store.GetColorDesc().has_value());

    D3D11_TEXTURE2D_DESC first{};
    first.Width = 64;
    D3D11_TEXTURE2D_DESC second{};
    second.Width = 32;

    // The first description noted is kept.
    store.NoteColorDesc(first);
    store.NoteColorDesc(second);
    ASSERT_TRUE(store.GetColorDesc().has_value());
    EXPECT_EQ(store.GetColorDesc()->Width, 64u);

    for (std::size_t i{ 0 }; i < k_ColorPairCount; ++i)
    {
        const auto index{ store.GetColorPairs().NoteCall({ .State = i == 1 ? PairState::NoTarget : PairState::Copied,
            .Phase = static_cast<std::uint8_t>(i) }, nullptr) };
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, i);
    }

    EXPECT_FALSE(store.GetColorPairs().NoteCall({ .State = PairState::Copied }, nullptr).has_value());
    EXPECT_EQ(store.GetColorPairs().GetNote(1).State, PairState::NoTarget);
    EXPECT_EQ(store.GetColorPairs().GetNote(3).Phase, 3);

    // The depth pairs are not touched by the color ones.
    EXPECT_EQ(store.GetDepthPairs().GetNoteCount(), 0u);

    store.ReleaseColorPairs();
    EXPECT_EQ(store.GetColorPairs().GetNoteCount(), 0u);
    EXPECT_FALSE(store.GetColorDesc().has_value());
}

TEST(EngineLayerTests, RenderStoreStencilProbeIsClearedWhenArmed)
{
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::StencilProbeInfo;
    using Engine::Render::Type::StencilReadStatus;

    Engine::Render::State::RenderStore store{};

    StencilProbeInfo info{};
    info.IsAttempted = true;
    info.Status = StencilReadStatus::MapFailed;
    info.Result = -2005270523;
    store.SetStencilProbe(info);
    EXPECT_TRUE(store.IsStencilProbeAttempted());
    EXPECT_EQ(store.GetStencilProbe().Status, StencilReadStatus::MapFailed);
    EXPECT_EQ(store.GetFirstPersonDepth(), nullptr);

    store.SetFirstPersonDriveActive(true);
    EXPECT_TRUE(store.IsFirstPersonDriveActive());

    store.RequestFirstPersonProbe();
    EXPECT_FALSE(store.StepFirstPersonProbe());     // armed: the stencil of the old probe is gone
    EXPECT_FALSE(store.IsStencilProbeAttempted());
    EXPECT_FALSE(store.GetStencilProbe().IsAttempted);

    store.CountFirstPersonCall(FirstPersonCaller::Pass1);
    store.EndHudFrame();
    EXPECT_TRUE(store.StepFirstPersonProbe());
    EXPECT_EQ(store.GetFirstPersonDepth(), nullptr);
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

TEST(EngineLayerTests, MaskWindowCountsFramesWithoutCopyPerPairAndTheMostPassCallsAndPublishesEvery120Frames)
{
    using Engine::Render::Type::AdvanceMaskWindow;
    using Engine::Render::Type::MaskWindow;
    using Engine::Render::Type::k_MaskWindowFrames;

    MaskWindow window{};
    EXPECT_EQ(k_MaskWindowFrames, 120u);

    // 119 frames publish nothing. Pair 0 misses every 10th frame, pair 1 never copies and the color pair always does. The calls go 1, 2 or 3.
    for (std::uint32_t i{ 0 }; i < k_MaskWindowFrames - 1; ++i)
    {
        const bool isCopied{ i % 10 != 0 };
        EXPECT_FALSE(AdvanceMaskWindow(window, { isCopied, false, true }, i == 50 ? 3u : 1u).has_value());
    }
    EXPECT_EQ(window.Frames, 119u);
    EXPECT_EQ(window.NoCopyFrames[0], 12u);
    EXPECT_EQ(window.NoCopyFrames[1], 119u);
    EXPECT_EQ(window.NoCopyFrames[2], 0u);
    EXPECT_EQ(window.MaxPassCalls, 3u);

    // The 120th publishes the window and starts a new one.
    const auto report{ AdvanceMaskWindow(window, { false, false, true }, 2) };
    ASSERT_TRUE(report.has_value());
    EXPECT_EQ(report->Frames, 120u);
    EXPECT_EQ(report->NoCopyFrames[0], 13u);
    EXPECT_EQ(report->NoCopyFrames[1], 120u);
    EXPECT_EQ(report->NoCopyFrames[2], 0u);
    EXPECT_EQ(report->MaxPassCalls, 3u);
    EXPECT_EQ(window.Frames, 0u);
    EXPECT_EQ(window.NoCopyFrames[0], 0u);
    EXPECT_EQ(window.MaxPassCalls, 0u);

    // The next window does not keep the maximum of the last one.
    for (std::uint32_t i{ 0 }; i < k_MaskWindowFrames - 1; ++i) EXPECT_FALSE(AdvanceMaskWindow(window, { true, true, true }, 1).has_value());
    const auto second{ AdvanceMaskWindow(window, { true, true, true }, 1) };
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->NoCopyFrames[0], 0u);
    EXPECT_EQ(second->NoCopyFrames[1], 0u);
    EXPECT_EQ(second->MaxPassCalls, 1u);
}

TEST(EngineLayerTests, RenderStoreMaskWindowOnlyCountsFramesWithTheMaskRequested)
{
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::k_MaskWindowFrames;

    Engine::Render::State::RenderStore store{};

    // Not requested: the calls and the frames are not counted.
    store.CountMaskPassCall();
    for (std::uint32_t i{ 0 }; i < k_MaskWindowFrames; ++i) store.EndHudFrame();
    EXPECT_FALSE(store.GetMaskWindowReport().has_value());

    // Requested: 120 frames, the first one copied the pair 0 and the color pair after two 0xB calls, the others without copy.
    store.SetFirstPersonMaskRequested(true);
    store.CountMaskPassCall();
    store.CountMaskPassCall();
    store.MarkMaskCopied(MaskPairKind::DepthB);
    store.MarkMaskCopied(MaskPairKind::Color);
    store.EndHudFrame();
    for (std::uint32_t i{ 1 }; i < k_MaskWindowFrames; ++i)
    {
        EXPECT_FALSE(store.GetMaskWindowReport().has_value());
        store.EndHudFrame();
    }

    const auto report{ store.GetMaskWindowReport() };
    ASSERT_TRUE(report.has_value());
    EXPECT_EQ(report->Frames, k_MaskWindowFrames);
    EXPECT_EQ(report->NoCopyFrames[0], k_MaskWindowFrames - 1);
    EXPECT_EQ(report->NoCopyFrames[1], k_MaskWindowFrames);
    EXPECT_EQ(report->NoCopyFrames[2], k_MaskWindowFrames - 1);
    EXPECT_EQ(report->MaxPassCalls, 2u);

    // Turning the mask off drops the report.
    store.SetFirstPersonMaskRequested(false);
    store.EndHudFrame();
    EXPECT_FALSE(store.GetMaskWindowReport().has_value());
}