module;

#include <d3d11.h>

export module Viewer.Map.System:MapService;

import :MapPass;
import :DynamicPass;
import :ZonePass;
import :RaycastPass;
import :FixturePass;
import :LimitsPass;
import :AimPass;

import Service.Logs.System;
import Platform.Render.Type;
import Platform.Render.System;
import Export.Tick.State;
import Viewer.Profiler.System;
import Viewer.Scene.System;
import Viewer.Options.State;
import std;

export namespace Viewer::Map::System
{
    template <typename T>
    concept RenderPass = requires(T pass, ID3D11DeviceContext* context,
        Platform::Render::System::GpuPipeline& pipeline)
    {
        { pass.Draw(context, pipeline) } -> std::same_as<void>;
        { pass.Release() } -> std::same_as<void>;
    };

    static_assert(RenderPass<MapPass>);
    static_assert(RenderPass<DynamicPass>);
    static_assert(RenderPass<ZonePass>);
    static_assert(RenderPass<RaycastPass>);
    static_assert(RenderPass<FixturePass>);
    static_assert(RenderPass<LimitsPass>);
    static_assert(RenderPass<AimPass>);

    // Renders the map frame: the static map, the collidables, the zones, the raycasts, the fixture arrows, the aim spheres and the map limits.
    // It uploads what changed in the tick, draws every pass and times the frame with the profiler.
    // note: Render runs on the game render thread. Suspend, Resume and Release may come from other threads, so they share a mutex.
    class MapService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using FrameContext = Platform::Render::Type::FrameContext;
        using GpuPipeline = Platform::Render::System::GpuPipeline;
        using GpuStateGuard = Platform::Render::System::GpuStateGuard;

        using TickStore = Export::Tick::State::TickStore;

        using FrameProfiler = Viewer::Profiler::System::FrameProfiler;
        using SceneService = Viewer::Scene::System::SceneService;
        using OptionsStore = Viewer::Options::State::OptionsStore;

    public:
        MapService(LogsService& logsService,
            TickStore& tickStore, SceneService& sceneService, OptionsStore& optionsStore) :
            m_LogsService(logsService),
            m_TickStore(tickStore), m_SceneService(sceneService), m_OptionsStore(optionsStore),
            m_GpuPipeline(logsService), m_MapPass(logsService),
            m_DynamicPass(logsService), m_ZonePass(logsService),
            m_RaycastPass(logsService), m_FixturePass(logsService), m_LimitsPass(logsService), m_AimPass(logsService) {}
        ~MapService() = default;

        MapService(const MapService&) = delete;
        auto operator=(const MapService&) -> MapService& = delete;

        // Draws one frame. Does nothing while suspended or while the free camera is inactive.
        // note: Releases the GPU resources first if the device changed.
        auto Render(const FrameContext& frame) -> void;

        // Releases every GPU resource. Call it before the device goes away.
        auto Release() -> void;

        // Stops rendering and drops the map data on the next frame, because the map is about to change.
        auto Suspend() -> void;

        auto Resume() -> void;

    private:
        LogsService& m_LogsService;
        TickStore& m_TickStore;
        SceneService& m_SceneService;
        OptionsStore& m_OptionsStore;

        GpuPipeline m_GpuPipeline;
        MapPass m_MapPass;
        DynamicPass m_DynamicPass;
        ZonePass m_ZonePass;
        RaycastPass m_RaycastPass;
        FixturePass m_FixturePass;
        LimitsPass m_LimitsPass;
        AimPass m_AimPass;
        FrameProfiler m_Profiler{};

        std::mutex m_Mutex{};
        bool m_IsSuspended{ false };
        bool m_IsMapResetPending{ false };

        // Device the resources were created on. Compared only, never used.
        const ID3D11Device* m_Device{ nullptr };

        auto Draw(const FrameContext& frame) -> void;

        // Releases what depends on the map and resets the scene.
        auto ReleaseMap() -> void;

        auto ReleaseAll() -> void;
    };
}