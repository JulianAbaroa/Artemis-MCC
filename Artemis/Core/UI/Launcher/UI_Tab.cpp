#include "pch.h"

#include "UI_Tab.h"

bool UI_Tab::s_Locked = false;
int UI_Tab::s_VisibleCount = 0;

bool UI_Tab::IsVisible() const 
{ 
    return m_Visible; 
}

void UI_Tab::ToggleVisible() 
{ 
    m_Visible = !m_Visible; 
    s_VisibleCount += m_Visible ? 1 : -1;
}

void UI_Tab::RequestReset() 
{ 
    m_ResetRequested = true; 
}

void UI_Tab::FirstDraw()
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 center = vp->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver,
        ImVec2(0.5f, 0.5f));

    ImGui::SetNextWindowSize(ImVec2(1360.0f, 800.0f),
        ImGuiCond_FirstUseEver);
}

void UI_Tab::ApplyResetIfRequested(const char* windowName)
{
    if (!m_ResetRequested) return;
    m_ResetRequested = false;

    ImGuiWindow* window = ImGui::FindWindowByName(windowName);
    if (window && window->DockId != 0)
    {
        ImGui::SetWindowDock(window, 0, ImGuiCond_Always);
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(
        ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
}