module;

#include <d3d11.h>

module Viewer.Map.System;
import :MapService;

import Viewer.Camera.Type;
import Viewer.Options.Type;
import Viewer.Profiler.Type;
import Viewer.Profiler.System;
import Viewer.Map.Type;
import Viewer.Style.Type;
import std;

namespace
{
    using Viewport = Viewer::Camera::Type::Viewport;
    using Flag = Viewer::Options::Type::Flag;
    using Scalar = Viewer::Options::Type::Scalar;
    using Cpu = Viewer::Profiler::Type::Cpu;
    using Gpu = Viewer::Profiler::Type::Gpu;
    using Counter = Viewer::Profiler::Type::Counter;
    using CpuTimer = Viewer::Profiler::System::CpuTimer;
    using FixturePassOptions = Viewer::Map::Type::FixturePassOptions;

    using Viewer::Style::Type::k_Background;
}

namespace Viewer::Map::System
{
    auto MapService::Render(const FrameContext& frame) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_IsSuspended) return;

        if (!frame.Device || !frame.Context ||
            !frame.RenderTarget || !frame.DepthStencil) return;

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
            return;
        }

        const auto tick = m_TickStore.Acquire();
        if (!tick) return;

        if (!m_GpuPipeline.Initialize(frame.Device)) return;

        if (tick->Map && !m_MapPass.IsUploaded())
        {
            m_MapPass.Upload(frame.Device, *tick->Map);
        }

        if (!m_MapPass.HasBuffer()) return;

        m_Profiler.BeginFrame(frame.Device, frame.Context);

        {
            CpuTimer frameTimer{ m_Profiler, Cpu::Frame };

            Viewport viewport{};
            viewport.Size.X = static_cast<float>(frame.Width);
            viewport.Size.Y = static_cast<float>(frame.Height);

            {
                CpuTimer timer{ m_Profiler, Cpu::Scene };
                m_SceneService.Update(tick, viewport);
            }

            const auto& palette = m_SceneService.GetPalette();
            const std::uint32_t selected = m_SceneService.GetSelected();

            {
                CpuTimer timer{ m_Profiler, Cpu::Dynamic };
                std::unordered_set<std::uint32_t> translucentHandles{};

                if (tick->Fixtures && m_OptionsStore.IsEnabled(Flag::FixtureShieldTranslucent))
                {
                    for (const auto& shield : tick->Fixtures->Shields) translucentHandles.insert(shield.Handle);
                }

                m_DynamicPass.Upload(frame.Device, frame.Context, tick->Collidables,
                    palette, selected, translucentHandles, tick->Generation);
            }

            {
                CpuTimer timer{ m_Profiler, Cpu::Zone };
                m_ZonePass.Upload(frame.Device, frame.Context, tick, palette);
            }

            {
                CpuTimer timer{ m_Profiler, Cpu::Raycast };
                m_RaycastPass.Upload(frame.Device, frame.Context, tick->Raycasts, tick->Generation);
            }

            {
                FixturePassOptions fixtureOptions{};
                fixtureOptions.TeleportLinks = m_OptionsStore.IsEnabled(Flag::FixtureTeleportLinks);
                fixtureOptions.LiftArrows = m_OptionsStore.IsEnabled(Flag::FixtureLiftArrows);
                fixtureOptions.ShieldArrows = m_OptionsStore.IsEnabled(Flag::FixtureShieldArrows);
                fixtureOptions.ArrowLength = m_OptionsStore.GetScalar(Scalar::FixtureArrowLength);

                m_FixturePass.Upload(frame.Device, frame.Context, tick->Fixtures, fixtureOptions);
            }

            {
                CpuTimer timer{ m_Profiler, Cpu::Aim };
                m_AimPass.Upload(frame.Device, frame.Context, tick->Aims, tick->Healths,
                    m_OptionsStore.IsEnabled(Flag::HealthTintSpheres),
                    selected, tick->Generation);
            }

            m_Profiler.SetCounter(Counter::DynamicInstances,
                m_DynamicPass.GetInstanceCount());
            m_Profiler.SetCounter(Counter::DynamicDraws,
                m_DynamicPass.GetDrawCount());
            m_Profiler.SetCounter(Counter::AimInstances,
                m_AimPass.GetInstanceCount());

            {
                CpuTimer timer{ m_Profiler, Cpu::Draw };
                this->Draw(frame);
            }
        }

        m_Profiler.Report(m_LogsService);
    }

    auto MapService::Draw(const FrameContext& frame) -> void
    {
        ID3D11DeviceContext* context = frame.Context;

        GpuStateGuard guard(context);

        m_Profiler.BeginGpu(context);

        context->ClearRenderTargetView(frame.RenderTarget, k_Background);
        context->ClearDepthStencilView(frame.DepthStencil,
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        D3D11_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(frame.Width);
        viewport.Height = static_cast<float>(frame.Height);
        viewport.MaxDepth = 1.0f;

        ID3D11RenderTargetView* renderTarget = frame.RenderTarget;
        context->OMSetRenderTargets(1, &renderTarget, frame.DepthStencil);
        context->RSSetViewports(1, &viewport);

        m_GpuPipeline.UpdateCamera(context, m_SceneService.GetViewProjection());

        m_Profiler.MarkGpu(context, Gpu::Clear);

        m_MapPass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Map);

        m_DynamicPass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Dynamic);

        m_ZonePass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Zone);

        m_RaycastPass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Raycast);

        m_FixturePass.Draw(context, m_GpuPipeline);

        m_AimPass.Draw(context, m_GpuPipeline);
        m_Profiler.MarkGpu(context, Gpu::Aim);

        m_DynamicPass.DrawTranslucent(context, m_GpuPipeline,
            m_OptionsStore.GetScalar(Scalar::ShieldOpacity));

        m_Profiler.EndGpu(context);
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
        m_FixturePass.Release();
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