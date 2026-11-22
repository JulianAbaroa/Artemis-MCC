module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;

import Common.Engine.Type;
import Engine.Base.Memory.Type;
import Engine.Base.Pov.Type;
import Engine.Control.Input.Type;
import Engine.Render.Type;
import Perception.Players.Type;
import DevTools.Viewer.Latency.Type;
import std;

namespace
{
    using Engine::Base::Memory::Type::GameCamera::GameCamera;
    using Engine::Base::Memory::Type::FrameClock::ClockSource;
    using Engine::Base::Memory::Type::FrameClock::InterpolationState;
    using Engine::Render::Type::DrawPlace;
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::SceneStatus;
    using Engine::Base::Pov::Type::PovStatus;
    using Engine::Control::Input::Type::PossessStatus;
    using Perception::Players::Type::Player;

    using Common::Engine::Type::k_NoHandle;

    using DevTools::Viewer::Latency::Type::LatencyReport;
    using DevTools::Viewer::Latency::Type::LatencySample;

    constexpr float k_DegreesPerRadian{ 180.0f / std::numbers::pi_v<float> };


    const ImVec4 k_Green{ 0.0f, 1.0f, 0.0f, 1.0f };
    const ImVec4 k_Orange{ 1.0f, 0.6f, 0.0f, 1.0f };
    const ImVec4 k_Red{ 1.0f, 0.0f, 0.0f, 1.0f };
    const ImVec4 k_Info{ 0.6f, 0.8f, 1.0f, 1.0f };

    // return: The words the telemetry gives for why the overlay was not drawn at the scene point.
    auto SceneReason(SceneStatus status) -> const char*
    {
        switch (status)
        {
        case SceneStatus::NoSceneCall:      return "no opaque color pass call in the frame (menu or pause)";
        case SceneStatus::OtherThread:      return "draws on another thread";
        case SceneStatus::NotReady:         return "render target not ready";
        case SceneStatus::TargetMismatch:   return "target is not the size of the back buffer";
        case SceneStatus::OtherDevice:      return "target is of another device";
        case SceneStatus::SplitViewport:    return "viewport is not the whole target (split screen)";
        case SceneStatus::Multisampled:     return "target has more than one sample";
        case SceneStatus::NotRequested:     return "Viewer inactive (no scene draw requested)";
        case SceneStatus::Declined:         return "Viewer declined (no game camera or nothing to draw)";
        case SceneStatus::Failed:           return "a handler failed";
        case SceneStatus::Drawn:            return "drawn";
        }

        return "?";
    }
}

namespace DevTools::Viewer::Hud::System
{
    auto HudService::Draw() -> void
    {
        if (!m_OverlayStore.IsVisible()) return;

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f),
            ImGuiCond_Always, ImVec2(0.0f, 0.0f));

        ImGui::SetNextWindowBgAlpha(0.2f);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("##overlay", nullptr, flags))
        {
            if (m_RenderStore.IsGameHudHidden()) ImGui::TextColored(k_Orange, "Game HUD hidden");

            this->DrawDefault();
        }

        ImGui::End();
    }

    auto HudService::DrawDefault() -> void
    {
        this->DrawFramerate();
        this->DrawTelemetry();

        const auto scene = m_SceneService.GetFrameScene(m_CollidableStore);
        this->DrawGameCamera(scene ? scene->Tick : nullptr);
    }

    auto HudService::DrawFramerate() -> void
    {
        const float framerate = ImGui::GetIO().Framerate;
        const int fps = static_cast<int>(framerate + 0.5f);

        const ImVec4 color = fps >= 45 ? k_Green : fps >= 30 ? k_Orange : k_Red;

        ImGui::Text("Framerate:");
        ImGui::SameLine();
        ImGui::TextColored(color, "%d", fps);
    }

    auto HudService::DrawGameCamera(const std::shared_ptr<const Tick>& tick) -> void
    {
        ImGui::Separator();

        const std::optional<GameCamera>& camera{ m_SceneService.GetGameCamera() };

        ImGui::Text("Game camera:");
        ImGui::SameLine();
        ImGui::TextColored(camera ? k_Green : k_Red, "%s", camera ? "valid" : "not valid");

        this->DrawOverlayPoint();
        this->DrawSceneColor();

        if (!camera) return;

        ImGui::Text("Position:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.2f %.2f %.2f", camera->Position.X, camera->Position.Y, camera->Position.Z);

        const float fovHorizontal{ 2.0f * std::atan(1.0f / camera->M00) * k_DegreesPerRadian };
        const float fovVertical{ 2.0f * std::atan(1.0f / camera->M11) * k_DegreesPerRadian };

        ImGui::Text("FOV:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.1f h  %.1f v deg", fovHorizontal, fovVertical);

        const float width{ static_cast<float>(m_RenderStore.GetWidth()) };
        const float height{ static_cast<float>(m_RenderStore.GetHeight()) };
        const float cameraAspect{ camera->M11 / camera->M00 };
        const float bufferAspect{ height > 0.0f ? width / height : 0.0f };

        // note: A matrix aspect that is not the one of the back buffer means the game draws the view in a part of it, or with its own aspect.
        const bool isAspectEqual{ std::abs(cameraAspect - bufferAspect) < 0.01f };

        ImGui::Text("Aspect:");
        ImGui::SameLine();
        ImGui::TextColored(isAspectEqual ? k_Green : k_Orange, "%.3f (back buffer %.0fx%.0f = %.3f)",
            cameraAspect, width, height, bufferAspect);

        ImGui::Text("Off-center:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.4f %.4f", camera->M20, camera->M21);

        const std::optional<float> eyeDistance{ tick ? m_SceneService.GetEyeDistance(*tick) : std::nullopt };

        ImGui::Text("Distance to View eye:");
        ImGui::SameLine();

        if (eyeDistance)
        {
            ImGui::TextColored(k_Info, "%.3f wu", *eyeDistance);
        }
        else
        {
            ImGui::TextDisabled("n/a (no View)");
        }

        this->DrawOverride();
        this->DrawFirstPersonHook();
        this->DrawPov(tick);
        this->DrawPossession(tick);
        this->DrawFirstPersonMask();
        this->DrawLatency();
    }

    auto HudService::DrawOverride() -> void
    {
        const bool isActive{ m_SceneService.IsOverrideActive() };

        ImGui::Text("Override:");
        ImGui::SameLine();
        ImGui::TextColored(isActive ? k_Green : k_Orange, "%s", isActive ? "on" : "off");

        // note: The hook cannot log on the game thread, so the faults it caught are shown here.
        const std::uint32_t faults{ m_SceneService.GetOverrideFaults() };
        if (faults > 0)
        {
            ImGui::Text("Override faults:");
            ImGui::SameLine();
            ImGui::TextColored(k_Red, "%u", faults);
        }

    }

    auto HudService::DrawOverlayPoint() -> void
    {
        ImGui::Text("draw:");
        ImGui::SameLine();

        if (!m_RenderStore.IsOpaquePassHooked())
        {
            ImGui::TextColored(k_Orange, "none (no hook of the opaque color pass)");
            return;
        }

        if (m_RenderStore.GetDrawPlace() == DrawPlace::Scene) ImGui::TextColored(k_Green, "scene");
        else ImGui::TextColored(k_Orange, "none (%s)", SceneReason(m_RenderStore.GetSceneStatus()));

        const std::uint32_t failures{ m_RenderStore.GetSceneFailures() };
        if (failures == 0) return;

        ImGui::SameLine();
        ImGui::TextColored(k_Red, "scene failures %u", failures);
    }

    auto HudService::DrawSceneColor() -> void
    {
        ImGui::Text("scene color scale:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.4g", m_RenderStore.GetSceneColorScale());
    }

    auto HudService::DrawFirstPersonHook() -> void
    {
        const bool isModelHooked{ m_RenderStore.IsFirstPersonModelHooked() };
        const bool isUnitHooked{ m_RenderStore.IsFirstPersonUnitHooked() };
        const bool isDepthPassHooked{ m_RenderStore.IsFirstPersonDepthPassHooked() };
        const bool isOpaqueHooked{ m_RenderStore.IsOpaquePassHooked() };

        ImGui::Text("First person hook:");
        ImGui::SameLine();

        if (!isModelHooked && !isUnitHooked && !isDepthPassHooked && !isOpaqueHooked)
        {
            ImGui::TextColored(k_Orange, "not installed (model, unit, depth pass, opaque pass)");
            return;
        }

        ImGui::TextColored(isModelHooked && isUnitHooked && isDepthPassHooked && isOpaqueHooked ? k_Green : k_Orange,
            "installed (model %s, unit %s, depth pass %s, opaque pass %s), third person: %s, fp unit: %s", isModelHooked ? "yes" : "no",
            isUnitHooked ? "yes" : "no", isDepthPassHooked ? "yes" : "no", isOpaqueHooked ? "yes" : "no", m_RenderStore.WasThirdPersonLastFrame() ? "yes" : "no",
            m_RenderStore.WasFirstPersonUnitLastFrame() ? "yes" : "no");

    }

    auto HudService::DrawPov(const std::shared_ptr<const Tick>& tick) -> void
    {
        const PovStatus status{ m_PovStatusStore.Read() };

        ImGui::Text("POV hook:");
        ImGui::SameLine();
        ImGui::TextColored(status.IsInstalled ? k_Green : k_Orange, "%s", status.IsInstalled ? "installed" : "not found");

        ImGui::Text("POV:");
        ImGui::SameLine();

        if (status.RequestedHandle == k_NoHandle)
        {
            ImGui::TextDisabled("off");
            return;
        }

        const Player* player{ tick && tick->Players ? tick->Players->Find(status.RequestedHandle) : nullptr };

        ImGui::TextColored(status.Faults == 0 ? k_Info : k_Red, "%s req 0x%08X engine 0x%08X local 0x%08X calls %u reasserts %u loadout acks %u faults %u%s",
            player ? player->Name.c_str() : "?", status.RequestedHandle, status.EngineHandle, status.LocalHandle, status.Calls, status.Reasserts, status.LoadoutAcks, status.Faults,
            m_SceneService.IsPovPlayerDead(status) ? " dead" : "");
    }

    auto HudService::DrawPossession(const std::shared_ptr<const Tick>& tick) -> void
    {
        const PossessStatus status{ m_PossessionStore.GetStatus() };

        ImGui::Text("Possess:");
        ImGui::SameLine();

        if (status.Player == k_NoHandle)
        {
            ImGui::TextDisabled("off");
            return;
        }

        const Player* player{ tick && tick->Players ? tick->Players->Find(status.Player) : nullptr };

        ImGui::TextColored(status.IsUnitMissing || status.Unsupported != 0 ? k_Orange : k_Info, "%s ticks %llu delivered %llu unsupported %llu%s",
            player ? player->Name.c_str() : "?", status.Ticks, status.Delivered, status.Unsupported, status.IsUnitMissing ? " no unit" : "");
    }

    auto HudService::DrawFirstPersonMask() -> void
    {
        ImGui::Text("FP mask:");
        ImGui::SameLine();

        if (!m_RenderStore.IsFirstPersonMaskRequested())
        {
            ImGui::TextDisabled("off");
            return;
        }

        // note: The mask is made of two pairs of copies, and either one is enough to hide the arms.
        const bool isCopied{ m_RenderStore.WasMaskCopiedLastFrame(MaskPairKind::DepthB) || m_RenderStore.WasMaskCopiedLastFrame(MaskPairKind::DepthZero) };

        if (isCopied) ImGui::TextColored(k_Green, "on");
        else ImGui::TextColored(k_Orange, "on, no copy");
    }

    auto HudService::DrawLatency() -> void
    {
        const LatencyReport report{ m_LatencyService.GetReport() };

        ImGui::Text("Frame clock:");
        ImGui::SameLine();

        if (!report.HasLast)
        {
            ImGui::TextDisabled("no frame yet");
            return;
        }

        const LatencySample& last{ report.Last };
        if (last.Clock.Source == ClockSource::None)
        {
            ImGui::TextColored(k_Red, "unreadable");
        }
        else
        {
            const bool isThread{ last.Clock.Source == ClockSource::ThreadContext };
            const char* interpolation{ last.Clock.Interpolation == InterpolationState::On ? "yes"
                : last.Clock.Interpolation == InterpolationState::Off ? "no" : "?" };

            char scene[48]{};
            if (last.SceneTick) std::snprintf(scene, sizeof(scene), "S %u lag %d", *last.SceneTick, last.Lag);
            else std::snprintf(scene, sizeof(scene), "S n/a");

            ImGui::TextColored(isThread ? k_Green : k_Orange, "TLS %s, T %u f %.3f interp %s, %s",
                isThread ? "yes" : "no, published ctx", last.Clock.Tick, last.Clock.Fraction, interpolation, scene);
        }

        ImGui::Text("Scene lag (%u f):", static_cast<unsigned>(report.Frames));
        ImGui::SameLine();
        ImGui::TextColored(report.Lag[0] == report.Frames ? k_Green : k_Orange, "0:%u 1:%u 2:%u 3:%u 4+:%u neg:%u max:%d",
            report.Lag[0], report.Lag[1], report.Lag[2], report.Lag[3], report.Lag[4], report.Negative, report.MaxLag);

        if (report.InterpExact + report.InterpExtrapolated + report.InterpNone > 0)
        {
            ImGui::Text("Interp (%u f):", static_cast<unsigned>(report.Frames));
            ImGui::SameLine();
            ImGui::TextColored(report.InterpNone == 0 ? k_Green : k_Orange, "exact %u  extrap %u  none %u",
                report.InterpExact, report.InterpExtrapolated, report.InterpNone);
        }

        ImGui::Text("Camera moved within a tick:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%u / %u", report.CameraMovedFrames, report.SameTickFrames);

        ImGui::Text("Interp probe (%u up to date, %u behind):", report.ProbeFrames, report.BehindFrames);
        ImGui::SameLine();

        if (report.ProbeFrames == 0)
        {
            ImGui::TextDisabled("n/a (Game mode, on foot and moving)");
            return;
        }

        ImGui::TextColored(k_Info, "to E(S) %.3f/%.3f  to E(S-1) %.3f/%.3f  to lerp %.3f/%.3f wu (p50/p99)",
            report.ToCurrent.P50, report.ToCurrent.P99, report.ToPrevious.P50, report.ToPrevious.P99, report.ToLerp.P50, report.ToLerp.P99);
    }

    auto HudService::DrawTelemetry() -> void
    {
        auto& telemetry = m_TelemetryStore;

        const float tickHz = telemetry.m_TickHz.load(std::memory_order_relaxed);
        const float presentHz = telemetry.m_PresentHz.load(std::memory_order_relaxed);
        const float ticksPerCall = telemetry.m_TicksPerCall.load(std::memory_order_relaxed);
        const float engineMs = telemetry.m_SimMs.load(std::memory_order_relaxed);
        const float averageSweepMs = telemetry.m_SweepMs.load(std::memory_order_relaxed);
        const std::uint32_t dropped = telemetry.m_DroppedOut.load(std::memory_order_relaxed);
        const float readWaitMs = telemetry.m_ReadWaitMs.load(std::memory_order_relaxed);
        const std::uint32_t readTimeouts = telemetry.m_ReadTimeoutsOut.load(std::memory_order_relaxed);

        ImGui::Separator();

        ImGui::Text("Tick:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.1f Hz", tickHz);

        ImGui::Text("Present:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.1f Hz", presentHz);

        ImGui::Text("Ticks/frame:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.2f", ticksPerCall);

        ImGui::Text("Engine:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.3f ms/tick", engineMs);

        // The game waits for Artemis on every tick, so the sweep sets the most ticks per second Artemis can follow.
        const float maxTickHz = averageSweepMs > 0.0f ? (1000.0f / averageSweepMs) : 0.0f;

        ImGui::Text("Sweep:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.3f ms", averageSweepMs);
        ImGui::SameLine();
        ImGui::Text("(max %.0f ticks/s)", maxTickHz);

        // Sparse mode (Q101): the ticks that are not decisions only do the light pass, and are not in the sweep above.
        const float lightHz = telemetry.m_LightHz.load(std::memory_order_relaxed);
        if (lightHz > 0.0f)
        {
            ImGui::Text("Light ticks:");
            ImGui::SameLine();
            ImGui::TextColored(k_Info, "%.0f /s", lightHz);
            ImGui::SameLine();
            ImGui::Text("(%.1f us each)", telemetry.m_LightUs.load(std::memory_order_relaxed));
        }

        const ImVec4 dropColor = dropped == 0 ? k_Green : ImVec4(1.0f, 0.4f, 0.0f, 1.0f);

        ImGui::Text("Dropped:");
        ImGui::SameLine();
        ImGui::TextColored(dropColor, "%u /s", dropped);

        ImGui::Text("Read wait:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.3f ms", readWaitMs);

        const ImVec4 timeoutColor = readTimeouts == 0 ? k_Green : ImVec4(1.0f, 0.4f, 0.0f, 1.0f);

        ImGui::Text("Read timeouts:");
        ImGui::SameLine();
        ImGui::TextColored(timeoutColor, "%u /s", readTimeouts);
    }
}