module;

#include <dxgiformat.h>

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;

import Common.Engine.Type;
import Engine.Base.Memory.Type;
import Engine.Render.Type;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Overlay.Type;
import std;

namespace
{
    using DevTools::Viewer::Overlay::Type::Mode;

    using Engine::Base::Memory::Type::GameCamera::GameCamera;
    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::HudHookInfo;

    using Common::Engine::Type::k_NoHandle;
    using DevTools::Viewer::Collidable::Type::ShareObjects;

    constexpr float k_DegreesPerRadian{ 180.0f / std::numbers::pi_v<float> };


    const ImVec4 k_Green{ 0.0f, 1.0f, 0.0f, 1.0f };
    const ImVec4 k_Orange{ 1.0f, 0.6f, 0.0f, 1.0f };
    const ImVec4 k_Red{ 1.0f, 0.0f, 0.0f, 1.0f };
    const ImVec4 k_Info{ 0.6f, 0.8f, 1.0f, 1.0f };

    auto FormatName(std::uint32_t format) -> const char*
    {
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:        return "R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return "R8G8B8A8_UNORM_SRGB";
        case DXGI_FORMAT_B8G8R8A8_UNORM:        return "B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return "B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_R10G10B10A2_UNORM:     return "R10G10B10A2_UNORM";
        case DXGI_FORMAT_R16G16B16A16_FLOAT:    return "R16G16B16A16_FLOAT";
        case DXGI_FORMAT_R11G11B10_FLOAT:       return "R11G11B10_FLOAT";
        default:                                return nullptr;
        }
    }

    auto ModeLabel(Mode mode) -> const char*
    {
        switch (mode)
        {
        case Mode::Default:    return "Telemetry";
        case Mode::Collidable: return "Collidable";
        case Mode::Health:     return "Health";
        case Mode::Fixture:    return "Fixture";
        case Mode::Affordance: return "Affordance";
        case Mode::View:       return "View";
        default:               return "?";
        }
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
            this->DrawNavBar();

            if (m_RenderStore.IsGameHudHidden()) ImGui::TextColored(k_Orange, "Game HUD hidden");

            ImGui::Separator();

            if (m_OverlayStore.GetMode() == Mode::Default)
            {
                this->DrawDefault();
            }
            else if (m_OverlayStore.GetMode() == Mode::View)
            {
                const auto scene = m_SceneService.GetFrameScene(m_CollidableStore);
                this->DrawViewPanel(scene ? scene->Tick : nullptr);
            }
            else
            {
                const auto scene = m_SceneService.GetFrameScene(m_CollidableStore);
                this->DrawSelectedPanel(scene ? scene->Tick : nullptr);
            }
        }

        ImGui::End();
    }

    auto HudService::DrawNavBar() -> void
    {
        ImGui::TextDisabled("<");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.45f, 0.78f, 0.96f, 1.0f), "%s",
            ModeLabel(m_OverlayStore.GetMode()));
        ImGui::SameLine();
        ImGui::TextDisabled(">");
    }

    auto HudService::DrawDefault() -> void
    {
        this->DrawFramerate();
        this->DrawTelemetry();

        const auto scene = m_SceneService.GetFrameScene(m_CollidableStore);
        this->DrawGameCamera(scene ? scene->Tick : nullptr);
    }

    auto HudService::DrawViewPanel(const std::shared_ptr<const Tick>& tick) -> void
    {
        // note: It follows the watched player, not the selection, so it needs no selected object.
        ViewHud::Draw(tick ? m_SceneService.GetObservedView(*tick) : nullptr);
    }

    auto HudService::DrawSelectedPanel(const std::shared_ptr<const Tick>& tick) -> void
    {
        const std::uint32_t handle = m_SelectionStore.GetSelected();

        if (handle == k_NoHandle)
        {
            ImGui::TextDisabled("No selection.");
            m_LastHandle = k_NoHandle;
            return;
        }

        if (handle != m_LastHandle)
        {
            m_OverlayStore.ResetPage();
            m_LastHandle = handle;
        }

        const float screenHeight = static_cast<float>(m_RenderStore.GetHeight());
        const float pageHeight = screenHeight * 0.5f;
        const float screenWidth = static_cast<float>(m_RenderStore.GetWidth());
        const float panelWidth = (std::min)((std::max)(screenWidth * 0.30f, 480.0f), screenWidth - 40.0f);

        ImGui::BeginChild("##panel_body", ImVec2(panelWidth, pageHeight), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        if (tick) this->DrawPanel(*tick, handle);
        else      ImGui::TextDisabled("No data.");

        const float contentHeight = ImGui::GetCursorPosY();

        int totalPages{ 1 };
        if (pageHeight > 0.0f)
        {
            totalPages = static_cast<int>(std::ceil(contentHeight / pageHeight));
        }
        if (totalPages < 1) totalPages = 1;

        m_OverlayStore.ClampPage(totalPages - 1);
        const int page = m_OverlayStore.GetPage();
        ImGui::SetScrollY(page * pageHeight);

        ImGui::EndChild();

        if (totalPages > 1)
        {
            ImGui::Separator();
            ImGui::TextDisabled("Page %d/%d  (Up/Down)", page + 1, totalPages);
        }
    }

    auto HudService::DrawPanel(const Tick& tick, std::uint32_t handle) -> void
    {
        switch (m_OverlayStore.GetMode())
        {
        case Mode::Collidable:
            CollidablesHud::Draw(ShareObjects(m_SceneService.GetFrameScene(m_CollidableStore)), handle);
            break;

        case Mode::Health:
            HealthsHud::Draw(tick, handle);
            break;

        case Mode::Fixture:
            FixturesHud::Draw(tick, handle);
            break;

        case Mode::Affordance:
            AffordancesHud::Draw(tick, handle);
            break;

        default:
            break;
        }
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

        if (!m_SceneService.IsGameCamera())
        {
            ImGui::Text("Game camera:");
            ImGui::SameLine();
            ImGui::TextDisabled("n/a (free camera)");
            this->DrawOverride();
            return;
        }

        const std::optional<GameCamera>& camera{ m_SceneService.GetGameCamera() };

        ImGui::Text("Game camera:");
        ImGui::SameLine();
        ImGui::TextColored(camera ? k_Green : k_Red, "%s", camera ? "valid" : "not valid (the View of the local player is used)");

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
        this->DrawHudHook();
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

        const std::optional<float> distance{ m_SceneService.GetOverrideDistance() };
        if (!isActive || !distance) return;

        ImGui::Text("Override error:");
        ImGui::SameLine();
        ImGui::TextColored(k_Info, "%.3f wu (asked eye to game camera)", *distance);
    }

    auto HudService::DrawOverlayPoint(bool isHooked) -> void
    {
        ImGui::Text("Overlay drawn:");
        ImGui::SameLine();

        const BeforeHudStatus status{ m_RenderStore.GetBeforeHudStatus() };

        if (!isHooked)
        {
            ImGui::TextColored(k_Orange, "at Present (HUD hook not installed)");
            return;
        }

        if (status == BeforeHudStatus::Drawn)
        {
            ImGui::TextColored(k_Green, "before HUD");
            return;
        }

        const char* reason{ "?" };

        switch (status)
        {
        case BeforeHudStatus::NoHudDraw:       reason = "no HUD draw in the frame"; break;
        case BeforeHudStatus::OtherThread:     reason = "HUD draws on another thread"; break;
        case BeforeHudStatus::NotReady:        reason = "render target not ready"; break;
        case BeforeHudStatus::TargetMismatch:  reason = "HUD target is not the size of the back buffer"; break;
        case BeforeHudStatus::NotBackBuffer:   reason = "HUD target is not the back buffer"; break;
        case BeforeHudStatus::OtherDevice:     reason = "HUD target is of another device"; break;
        case BeforeHudStatus::SplitViewport:   reason = "viewport is not the whole target (split screen)"; break;
        case BeforeHudStatus::HudHidden:       reason = "game HUD hidden"; break;
        case BeforeHudStatus::Declined:        reason = "Viewer declined (overlay off, no game camera or nothing to draw)"; break;
        case BeforeHudStatus::Failed:          reason = "a handler failed, the frame was not redrawn"; break;
        default:                               break;
        }

        ImGui::TextColored(k_Info, "at Present (%s)", reason);
    }

    auto HudService::DrawHudHook() -> void
    {
        const HudHookInfo info{ m_RenderStore.GetHudHook() };

        this->DrawOverlayPoint(info.IsHooked);

        ImGui::Text("HUD hook:");
        ImGui::SameLine();

        if (!info.IsHooked)
        {
            ImGui::TextColored(k_Orange, "not installed");
            return;
        }

        if (!info.HasSample)
        {
            ImGui::TextDisabled("no HUD draw yet");
            return;
        }

        if (!info.IsSameThread)
        {
            ImGui::TextColored(k_Orange, "thread other, %u calls/frame (no D3D read)", info.CallsPerFrame);
            return;
        }

        char target[96]{};

        if (info.HasTarget)
        {
            const char* name{ FormatName(info.Format) };

            if (name != nullptr) std::snprintf(target, sizeof(target), "%ux%u %s", info.Width, info.Height, name);
            else std::snprintf(target, sizeof(target), "%ux%u fmt %u", info.Width, info.Height, info.Format);
        }
        else
        {
            std::snprintf(target, sizeof(target), "none");
        }

        char viewport[64]{};

        if (info.HasViewport)
        {
            std::snprintf(viewport, sizeof(viewport), "%.0f,%.0f %.0fx%.0f",
                info.ViewportX, info.ViewportY, info.ViewportWidth, info.ViewportHeight);
        }
        else
        {
            std::snprintf(viewport, sizeof(viewport), "none");
        }

        ImGui::TextColored(info.IsBackBuffer ? k_Green : k_Orange, "thread same, %u calls/frame, RTV %s, back buffer %s, viewport %s",
            info.CallsPerFrame, target, info.IsBackBuffer ? "yes" : "no", viewport);
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