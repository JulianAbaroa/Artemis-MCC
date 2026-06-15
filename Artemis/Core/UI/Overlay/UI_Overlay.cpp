#include "pch.h"

#include "UI_Overlay.h"

#include "Core/Types/Tick/Tick.h"

#include "Core/States/Other/Render/State_Render.h"
#include "Core/States/Other/Telemetry/State_Telemetry.h"
#include "Core/States/Other/Render/State_OverlayMode.h"
#include "Core/States/Other/Render/State_Selection.h"

#include "Core/UI/Overlay/Panels/OverlayPanel_Collidable.h"
#include "Core/UI/Overlay/Panels/OverlayPanel_Vitality.h"
#include "Core/UI/Overlay/Panels/OverlayPanel_Fixture.h"
#include "Core/UI/Overlay/Panels/OverlayPanel_Affordance.h"

#include "External/imgui/imgui.h"

#include <cmath>

namespace
{
    const char* ModeLabel(OverlayMode mode)
    {
        switch (mode)
        {
        case OverlayMode::Default:    return "Telemetry";
        case OverlayMode::Collidable: return "Collidable";
        case OverlayMode::Vitality:   return "Vitality";
        case OverlayMode::Fixture:    return "Fixture";
        case OverlayMode::Affordance: return "Affordance";
        default:                      return "?";
        }
    }
}

void UI_Overlay::Draw(std::shared_ptr<const Tick> tick)
{
    if (!m_IsVisible) return;

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f),
        ImGuiCond_Always, ImVec2(0.0f, 0.0f));

    ImGui::SetNextWindowBgAlpha(0.2f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("##overlay", nullptr, flags);

    this->DrawNavBar();
    ImGui::Separator();

    const OverlayMode mode = m_Deps.State_OverlayMode.Get();

    if (mode == OverlayMode::Default)
        this->DrawDefault();
    else
        this->DrawSelectedPanel(tick);

    ImGui::End();
}

void UI_Overlay::DrawNavBar()
{
    const OverlayMode mode = m_Deps.State_OverlayMode.Get();

    ImGui::TextDisabled("<");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.45f, 0.78f, 0.96f, 1.0f), "%s",
        ModeLabel(mode));
    ImGui::SameLine();
    ImGui::TextDisabled(">");
}

void UI_Overlay::DrawDefault()
{
    this->DrawFPS();
    this->DrawTelemetry();
}

void UI_Overlay::DrawSelectedPanel(const std::shared_ptr<const Tick>& tick)
{
    const uint32_t handle = m_Deps.State_Selection.GetSelected();

    if (handle == kNoSelection)
    {
        ImGui::TextDisabled("No selection.");
        m_LastHandle = kNoSelection;
        return;
    }

    if (handle != m_LastHandle)
    {
        m_Deps.State_OverlayMode.ResetPage();
        m_LastHandle = handle;
    }

    const float screenH = static_cast<float>(m_Deps.State_Render.GetHeight());
    const float pageH = screenH * 0.5f;

    const int page = m_Deps.State_OverlayMode.GetPage();

    ImGui::BeginChild("##panel_body", ImVec2(0.0f, pageH), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    switch (m_Deps.State_OverlayMode.Get())
    {
    case OverlayMode::Collidable:
        OverlayPanel_Collidable::Draw(tick, handle);
        break;
    case OverlayMode::Vitality:
        OverlayPanel_Vitality::Draw(tick, handle);
        break;
    case OverlayMode::Fixture:
        OverlayPanel_Fixture::Draw(tick, handle);
        break;
    case OverlayMode::Affordance:
        OverlayPanel_Affordance::Draw(tick, handle);
        break;
    default:
        break;
    }

    const float contentH = ImGui::GetCursorPosY();
    int totalPages = 1;
    if (pageH > 0.0f)
        totalPages = static_cast<int>(std::ceil(contentH / pageH));
    if (totalPages < 1) totalPages = 1;

    m_Deps.State_OverlayMode.ClampPage(totalPages - 1);
    const int clampedPage = m_Deps.State_OverlayMode.GetPage();
    ImGui::SetScrollY(clampedPage * pageH);

    ImGui::EndChild();

    if (totalPages > 1)
    {
        ImGui::Separator();
        ImGui::TextDisabled("Page %d/%d  (Up/Down)",
            clampedPage + 1, totalPages);
    }
}

void UI_Overlay::DrawFPS()
{
    int fps = m_Deps.State_Render.GetFramerate();

    ImVec4 fpsColor = fps >= 45 ?
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : fps >= 30 ?
        ImVec4(1.0f, 0.6f, 0.0f, 1.0f) :
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    ImGui::Text("Framerate:");
    ImGui::SameLine();
    ImGui::TextColored(fpsColor, "%d", fps);
}

void UI_Overlay::DrawTelemetry()
{
    auto& st = m_Deps.State_Telemetry;

    float tickHz = st.m_TickHz.load(std::memory_order_relaxed);
    float presentHz = st.m_PresentHz.load(std::memory_order_relaxed);
    float avgTickMs = st.m_SimMs.load(std::memory_order_relaxed);
    float avgSweepMs = st.m_SweepMs.load(std::memory_order_relaxed);

    ImGui::Separator();

    ImGui::Text("Tick:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.1f Hz", tickHz);

    ImGui::Text("Present:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.1f Hz", presentHz);

    float intervalMs = tickHz > 0.0f ? (1000.0f / tickHz) : 16.67f;
    float budgetMs = intervalMs - avgTickMs;
    if (budgetMs < 0.0f) budgetMs = 0.0f;

    ImVec4 sweepColor =
        (budgetMs > 0.0f && avgSweepMs < budgetMs * 0.5f) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
        : (budgetMs > 0.0f && avgSweepMs < budgetMs * 0.9f) ? ImVec4(1.0f, 0.6f, 0.0f, 1.0f)
        : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    ImGui::Text("Sweep:");
    ImGui::SameLine();
    ImGui::TextColored(sweepColor, "%.3f ms", avgSweepMs);
    ImGui::SameLine();
    ImGui::Text("/ budget %.2f ms", budgetMs);

    uint32_t dropped = st.m_DroppedOut.load(std::memory_order_relaxed);

    ImVec4 dropColor = dropped == 0
        ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
        : ImVec4(1.0f, 0.4f, 0.0f, 1.0f);

    ImGui::Text("Dropped:");
    ImGui::SameLine();
    ImGui::TextColored(dropColor, "%u /s", dropped);
}