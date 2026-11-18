module;

#include <d3d11.h>

module DevTools.Viewer.Map.System;
import :MapService;

import Common.Semantic.Type;
import Views.Raycast.Type;
import Views.View.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Profiler.Type;
import DevTools.Viewer.Profiler.System;
import DevTools.Viewer.Map.Type;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using DevTools::Viewer::Camera::Type::Viewport;
    using DevTools::Viewer::Collidable::Type::ShareObjects;
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Options::Type::Scalar;
    using DevTools::Viewer::Profiler::Type::Cpu;
    using DevTools::Viewer::Profiler::Type::Gpu;
    using DevTools::Viewer::Profiler::Type::Counter;
    using DevTools::Viewer::Profiler::System::CpuTimer;
    using DevTools::Viewer::Profiler::System::FrameProfiler;
    using DevTools::Viewer::Map::Type::FixturePassOptions;
    using DevTools::Viewer::Map::Type::LimitsPassOptions;
    using Common::Semantic::Type::Function;
    using Views::Raycast::Type::Raycasts;
    using Views::View::System::RaycastsOf;

    using DevTools::Viewer::Style::Type::k_Background;

    // Ends the GPU timing of the frame when it goes out of scope.
    class GpuTimingScope
    {
    public:
        GpuTimingScope(FrameProfiler& profiler, ID3D11DeviceContext* context) : m_Profiler(profiler), m_Context(context) {}
        ~GpuTimingScope() { m_Profiler.EndGpu(m_Context); }

        GpuTimingScope(const GpuTimingScope&) = delete;
        auto operator=(const GpuTimingScope&) -> GpuTimingScope& = delete;

    private:
        FrameProfiler& m_Profiler;
        ID3D11DeviceContext* m_Context{ nullptr };
    };
}

namespace DevTools::Viewer::Map::System
{
    auto MapService::Render(const FrameContext& frame) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        this->RenderFrame(frame, false);
    }

    auto MapService::RenderBeforeHud(const FrameContext& frame) -> bool
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // note: Decided before the scene update, so a frame that declines still gets its one update at Present.
        if (!this->IsOverlayActive()) return false;

        return this->RenderFrame(frame, true);
    }

    auto MapService::RenderFrame(const FrameContext& frame, bool isBeforeHud) -> bool
    {
        if (m_IsSuspended) return false;

        if (!frame.Device || !frame.Context ||
            !frame.RenderTarget || !frame.DepthStencil) return false;

        if (frame.Device != m_Device)
        {
            this->ReleaseAll();
            m_Device = frame.Device;
        }

        if (m_IsMapResetPending)
        {
            this->ReleaseMap();
            m_IsMapResetPending = false;
        }

        if (!m_SceneService.IsActive())
        {
            m_SceneService.Deactivate();
            return false;
        }

        // note: One acquire for the whole frame. The camera, the passes and the meshes all come from the same tick (D294).
        const auto scene = m_CollidableStore.Acquire();
        if (!scene || !scene->Tick) return false;
        const auto& tick = scene->Tick;

        if (!m_GpuPipeline.Initialize(frame.Device)) return false;

        if (tick->Map && !m_MapPass.IsUploaded())
        {
            m_MapPass.Upload(frame.Device, *tick->Map);
        }

        if (!m_MapPass.HasBuffer()) return false;

        m_Profiler.BeginFrame(frame.Device, frame.Context);

        {
            CpuTimer frameTimer{ m_Profiler, Cpu::Frame };

            Viewport viewport{};
            viewport.Size.X = static_cast<float>(frame.Width);
            viewport.Size.Y = static_cast<float>(frame.Height);

            {
                CpuTimer timer{ m_Profiler, Cpu::Scene };
                m_SceneService.Update(scene, viewport);
            }

            // note: The mode may have changed since RenderBeforeHud decided. The frame still counts as handled (the scene was updated once),
            // but nothing is drawn over the game frame that is not an overlay.
            if (isBeforeHud && !this->IsOverlayActive()) return true;

            const auto& palette = m_SceneService.GetPalette();
            const std::uint32_t selected = m_SceneService.GetSelected();
            const auto& hidden = m_SceneService.GetHiddenHandles();

            {
                CpuTimer timer{ m_Profiler, Cpu::Dynamic };
                std::unordered_set<std::uint32_t> translucentHandles{};

                // note: Every Barrier entity, drop shield bubbles and portable shields included (D91).
                if (tick->Entities && m_OptionsStore.IsEnabled(Flag::FixtureShieldTranslucent))
                {
                    for (const auto& entity : tick->Entities->Rows)
                    {
                        if (entity.Function == Function::Barrier) translucentHandles.insert(entity.Handle);
                    }
                }

                // note: The debug collidables are not part of the Tick. They come with it in the scene.
                m_DynamicPass.Upload(frame.Device, frame.Context, ShareObjects(scene),
                    palette, selected, translucentHandles, hidden);
            }

            {
                CpuTimer timer{ m_Profiler, Cpu::Zone };
                m_ZonePass.Upload(frame.Device, frame.Context, tick, palette);
            }

            {
                CpuTimer timer{ m_Profiler, Cpu::Raycast };
                const auto* view{ m_SceneService.GetObservedView(*tick) };
                const Raycasts raycasts{ view ? RaycastsOf(*view) : Raycasts{} };
                m_RaycastPass.Upload(frame.Device, frame.Context, view ? &raycasts : nullptr, tick->Generation);

                // note: With the option off the pass gets no View and draws nothing.
                const bool showView{ m_OptionsStore.IsEnabled(Flag::PlayerView) };
                m_ViewPass.Upload(frame.Device, frame.Context, showView ? view : nullptr, tick.get(), tick->Generation);
            }

            {
                FixturePassOptions fixtureOptions{};
                fixtureOptions.TeleportLinks = m_OptionsStore.IsEnabled(Flag::FixtureTeleportLinks);
                fixtureOptions.LiftArrows = m_OptionsStore.IsEnabled(Flag::FixtureLiftArrows);
                fixtureOptions.ShieldArrows = m_OptionsStore.IsEnabled(Flag::FixtureShieldArrows);
                fixtureOptions.ArrowLength = m_OptionsStore.GetScalar(Scalar::FixtureArrowLength);

                m_FixturePass.Upload(frame.Device, frame.Context, tick, fixtureOptions);
            }

            {
                LimitsPassOptions limitsOptions{};
                limitsOptions.KillVolumes = m_OptionsStore.IsEnabled(Flag::LimitKillVolumes);
                limitsOptions.SafeVolumes = m_OptionsStore.IsEnabled(Flag::LimitSafeVolumes);
                limitsOptions.SoftCeilings = m_OptionsStore.IsEnabled(Flag::LimitSoftCeilings);
                limitsOptions.Opacity = m_OptionsStore.GetScalar(Scalar::LimitOpacity);

                m_LimitsPass.Upload(frame.Device, frame.Context, tick->Limits, tick->Designs, limitsOptions);
            }

            {
                CpuTimer timer{ m_Profiler, Cpu::Aim };
                // note: With the option off the pass gets no aims and draws nothing.
                m_AimPass.Upload(frame.Device, frame.Context,
                    m_OptionsStore.IsEnabled(Flag::AimSpheres) ? tick->Aims : nullptr, tick->VitalityDetails,
                    m_OptionsStore.IsEnabled(Flag::HealthTintSpheres),
                    selected, hidden, tick->Generation);
            }

            m_Profiler.SetCounter(Counter::DynamicInstances,
                m_DynamicPass.GetInstanceCount());
            m_Profiler.SetCounter(Counter::DynamicDraws,
                m_DynamicPass.GetDrawCount());
            m_Profiler.SetCounter(Counter::AimInstances,
                m_AimPass.GetInstanceCount());

            {
                CpuTimer timer{ m_Profiler, Cpu::Draw };
                this->Draw(frame, isBeforeHud);
            }
        }

        m_Profiler.Report(m_Log);
        return true;
    }

    auto MapService::IsOverlayActive() const -> bool
    {
        return m_SceneService.IsGameCamera() && m_OptionsStore.IsEnabled(Flag::Overlay);
    }

    auto MapService::Draw(const FrameContext& frame, bool isBeforeHud) -> void
    {
        ID3D11DeviceContext* context = frame.Context;

        GpuStateGuard guard(context);

        m_Profiler.BeginGpu(context);

        // Closes the GPU timing even if a draw throws, so the next frame does not begin a query that is still open.
        const GpuTimingScope gpuTiming{ m_Profiler, context };

        // note: With the overlay the frame of the game stays under what the Viewer draws, so the color is not cleared. The depth is the
        // Viewer's own and is always cleared to 0, the far value of the reversed depth.
        const bool isOverlay{ this->IsOverlayActive() };

        // note: Never before the HUD of the game: what is under it is the game frame.
        if (!isOverlay && !isBeforeHud) context->ClearRenderTargetView(frame.RenderTarget, k_Background);
        context->ClearDepthStencilView(frame.DepthStencil,
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

        D3D11_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(frame.Width);
        viewport.Height = static_cast<float>(frame.Height);
        viewport.MaxDepth = 1.0f;

        ID3D11RenderTargetView* renderTarget = frame.RenderTarget;
        context->OMSetRenderTargets(1, &renderTarget, frame.DepthStencil);
        context->RSSetViewports(1, &viewport);

        m_GpuPipeline.UpdateCamera(context, m_SceneService.GetViewProjection());

        m_Profiler.MarkGpu(context, Gpu::Clear);

        if (isOverlay) m_MapPass.DrawOverlay(context, m_GpuPipeline, m_OptionsStore.IsEnabled(Flag::OverlayWireframe));
        else m_MapPass.Draw(context, m_GpuPipeline, m_OptionsStore.IsEnabled(Flag::MapCollision));
        m_Profiler.MarkGpu(context, Gpu::Map);

        m_DynamicPass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Dynamic);

        m_ZonePass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Zone);

        m_RaycastPass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Raycast);

        m_ViewPass.Draw(context, m_GpuPipeline);

        m_FixturePass.Draw(context, m_GpuPipeline);

        m_AimPass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Aim);

        m_LimitsPass.Draw(context, m_GpuPipeline);

        m_DynamicPass.DrawTranslucent(context, m_GpuPipeline,
            m_OptionsStore.GetScalar(Scalar::ShieldOpacity));

    }

    auto MapService::Suspend() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_IsSuspended = true;
        m_IsMapResetPending = true;

        m_SceneService.ClearSelection();
    }

    auto MapService::Resume() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_IsSuspended = false;
    }

    auto MapService::Release() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        this->ReleaseAll();
        m_Device = nullptr;
    }

    auto MapService::ReleaseMap() -> void
    {
        m_MapPass.Release();
        m_DynamicPass.Release();
        m_ZonePass.Release();
        m_RaycastPass.Release();
        m_ViewPass.Release();
        m_FixturePass.Release();
        m_LimitsPass.Release();
        m_AimPass.Release();
        m_SceneService.Reset();
    }

    auto MapService::ReleaseAll() -> void
    {
        this->ReleaseMap();
        m_Profiler.Release();
        m_GpuPipeline.Release();
    }
}