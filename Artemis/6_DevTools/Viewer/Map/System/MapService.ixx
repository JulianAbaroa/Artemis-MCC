module;

#include <d3d11.h>

export module DevTools.Viewer.Map.System:MapService;

import :MapPass;
import :DynamicPass;
import :ZonePass;
import :RaycastPass;
import :FixturePass;
import :LimitsPass;
import :AimPass;
import :ViewPass;
import :ObsPass;

import Common.Log.System;
import Engine.Base.Lifecycle.State;
import Engine.Base.Memory.Type;
import Engine.Render.Type;
import Engine.Render.System;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Interpolation.Type;
import DevTools.Viewer.Latency.Type;
import DevTools.Viewer.Latency.System;
import DevTools.Viewer.Map.Type;
import DevTools.Viewer.Profiler.System;
import DevTools.Viewer.Scene.System;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Options.State;
import std;

export namespace DevTools::Viewer::Map::System
{
    template <typename T>
    concept RenderPass = requires(T pass, ID3D11DeviceContext* context,
        Engine::Render::System::GpuPipeline& pipeline)
    {
        { pass.Draw(context, pipeline) } -> std::same_as<void>;
        { pass.Release() } -> std::same_as<void>;
    };

    // MapPass draws with an extra flag (the wireframe), so it only has to release.
    static_assert(requires(MapPass pass) { { pass.Release() } -> std::same_as<void>; });
    static_assert(RenderPass<DynamicPass>);
    static_assert(RenderPass<ZonePass>);
    static_assert(RenderPass<RaycastPass>);
    static_assert(RenderPass<FixturePass>);
    static_assert(RenderPass<LimitsPass>);
    static_assert(RenderPass<AimPass>);
    static_assert(RenderPass<ViewPass>);
    static_assert(RenderPass<ObsPass>);

    // Renders the overlay frame: the static map collision, the collidables, the zones, the raycasts, the View of the watched player, the observation arrows of the agents (VW.17), the fixture arrows, the aim spheres and the map limits.
    // It uploads what changed in the tick, draws every pass and times the frame with the profiler.
    // note: Render runs on the game render thread. Suspend, Resume and Release may come from other threads, so they share a mutex.
    class MapService
    {
    private:
        using Log = Common::Log::System::Log;

        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using FrameClock = Engine::Base::Memory::Type::FrameClock::FrameClock;

        using FrameContext = Engine::Render::Type::FrameContext;
        using GpuPipeline = Engine::Render::System::GpuPipeline;
        using GpuStateGuard = Engine::Render::System::GpuStateGuard;

        using CollidableScene = DevTools::Viewer::Collidable::Type::CollidableScene;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;

        using FrameScenePick = DevTools::Viewer::Map::Type::FrameScenePick;
        using FramePoses = DevTools::Viewer::Interpolation::Type::FramePoses;

        using FrameProfiler = DevTools::Viewer::Profiler::System::FrameProfiler;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using LatencyService = DevTools::Viewer::Latency::System::LatencyService;
        using ScenePasses = DevTools::Viewer::Options::Type::ScenePasses;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;

    public:
        // param collidableStore: The scene drawn, with the Tick it belongs to (D294).
        // param latencyService: Told, once per frame, what the overlay draws against the clock of the game (VW.5c).
        // param lifecycleStore: Read to not wait for a scene while the pipeline is being torn down.
        MapService(Log& log, CollidableStore& collidableStore,
            SceneService& sceneService, OptionsStore& optionsStore, LatencyService& latencyService,
            LifecycleStore& lifecycleStore) :
            m_Log(log), m_CollidableStore(collidableStore),
            m_SceneService(sceneService), m_OptionsStore(optionsStore), m_LatencyService(latencyService),
            m_LifecycleStore(lifecycleStore),
            m_GpuPipeline(log), m_MapPass(log),
            m_DynamicPass(log), m_ZonePass(log),
            m_RaycastPass(log), m_ViewPass(log), m_ObsPass(log), m_FixturePass(log), m_LimitsPass(log), m_AimPass(log) {}
        ~MapService() = default;

        MapService(const MapService&) = delete;
        auto operator=(const MapService&) -> MapService& = delete;

        // Draws the 3D passes of the overlay over the game frame, the only way they are drawn. Does nothing while suspended or while the Viewer is inactive.
        // note: Called from the scene event of Engine.Render, at the exit of the opaque color pass of the game, so it is also the one scene update of the
        // frame (camera, palette, selection) that the labels and panels of Present read. Releases the GPU resources first if the device changed.
        // return: True if it drew. When false the scene was not updated for drawing (the SceneService has no frame update).
        auto RenderScene(const FrameContext& frame) -> bool;

        // Releases every GPU resource. Call it before the device goes away.
        auto Release() -> void;

        // Stops rendering and drops the map data on the next frame, because the map is about to change.
        auto Suspend() -> void;

        auto Resume() -> void;

    private:
        Log& m_Log;
        CollidableStore& m_CollidableStore;
        SceneService& m_SceneService;
        OptionsStore& m_OptionsStore;
        LatencyService& m_LatencyService;
        LifecycleStore& m_LifecycleStore;

        GpuPipeline m_GpuPipeline;
        MapPass m_MapPass;
        DynamicPass m_DynamicPass;
        ZonePass m_ZonePass;
        RaycastPass m_RaycastPass;
        ViewPass m_ViewPass;
        ObsPass m_ObsPass;
        FixturePass m_FixturePass;
        LimitsPass m_LimitsPass;
        AimPass m_AimPass;
        FrameProfiler m_Profiler{};

        std::mutex m_Mutex{};
        bool m_IsSuspended{ false };
        bool m_IsMapResetPending{ false };

        // Engine tick of the previous frame, to not wait for a scene while the game is paused (the tick does not advance).
        bool m_IsLastClockTickKnown{ false };
        std::uint32_t m_LastClockTick{ 0 };

        // Poses the objects are drawn with in this frame, copied to the SceneService for what is drawn apart from the passes (VW.5e).
        FramePoses m_FramePoses{};

        // Device the resources were created on. Compared only, never used.
        const ID3D11Device* m_Device{ nullptr };

        // Chooses the scene of the frame (D294, VW.5d): the one of the tick the game drew (T - 1 of the clock), else the newest.
        // The game draws the state of the tick before the one its clock says (measured in VW.5c at 60 fps). If that scene is not published yet and
        // the scenes arrive one tick at a time, it waits up to k_FrameSceneWaitLimit for it, then takes the nearest one not newer than the target.
        // note: Without a clock the newest scene is taken and nothing waits.
        // note: The caller holds the mutex of this service, which the AI thread never takes, and the wait holds no lock of the store.
        auto AcquireFrameScene(const FrameClock& clock) -> FrameScenePick;

        // Finds the poses of the frame (VW.5e, D330): over the game frame, with the game interpolating and the scene of T - 1 drawn, each dynamic
        // object goes between its pose in T - 1 and in T by the tick fraction of the clock, or on from T - 2 when T is not in the history. Otherwise
        // the poses are empty and everything is drawn as the scene shows it. Gives them to the SceneService. The caller holds the mutex.
        // note: Reads the history of the store (a short mutex to copy a pointer each). Waits for nothing: T is used only if it is already there.
        auto UpdateFramePoses(const CollidableScene& scene, const FrameClock& clock, const FrameScenePick& pick) -> void;

        // Records the frame for the latency measurement, after the scene update. The caller holds the mutex.
        auto RecordLatency(const CollidableScene& scene, const FrameClock& clock, const FrameScenePick& pick) -> void;

        // Draws the passes of the frame over the game frame, under one GPU state guard that puts back what the game had bound.
        // param passes: The debug overlay (every pass) or, with it off, the presentation path: the depth of the map (invisible, so the walls hide the arrows)
        // and the observation arrows. The depth is cleared and the mask updated in both.
        auto Draw(const FrameContext& frame, const ScenePasses& passes) -> void;

        // Releases what depends on the map and resets the scene.
        auto ReleaseMap() -> void;

        auto ReleaseAll() -> void;
    };
}