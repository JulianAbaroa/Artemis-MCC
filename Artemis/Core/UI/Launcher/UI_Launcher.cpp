#include "pch.h"

#include "UI_Launcher.h"

#include "Core/UI/Launcher/UI_Tab.h"

#include "Resources/Icons/IconData.h"
#include "Core/UI/Utils/Icon/IconLoader.h"
#include "Core/States/Other/Render/State_Render.h"

#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"

bool UI_Launcher::s_IsVisible = false;

void UI_Launcher::Draw()
{
    if (!m_IsVisible) return;

    if (!m_IsLoaded) this->LoadIcons();

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));

    ImGui::SetNextWindowFocus();

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

    ImGui::Begin("##launcher", nullptr, flags);

    const IconTexture* icons[] = {
        &m_Icons.Objects,
        &m_Icons.Players,
        &m_Icons.Settings,
        &m_Icons.Scanner,
        &m_Icons.Logs,
    };

    for (size_t i = 0; i < m_Deps.Tabs.size(); i++)
    {
        if (i > 0) ImGui::SameLine();
        ImGui::PushID((int)i);
        DrawToggleButton(*icons[i], *m_Deps.Tabs[i]);
        ImGui::PopID();
    }

    ImGui::End();

    ImGui::PopStyleVar();
}

void UI_Launcher::LoadIcons()
{
    auto device = m_Deps.State_Render.GetDevice();

    LoadIconFromMemory(device, Icons::Data::Objects,
        Icons::Data::ObjectsSize, m_Icons.Objects);

    LoadIconFromMemory(device, Icons::Data::Players,
        Icons::Data::PlayersSize, m_Icons.Players);

    LoadIconFromMemory(device, Icons::Data::Settings,
        Icons::Data::SettingsSize, m_Icons.Settings);

    LoadIconFromMemory(device, Icons::Data::Scanner,
        Icons::Data::ScannerSize, m_Icons.Scanner);

    LoadIconFromMemory(device, Icons::Data::Logs,
        Icons::Data::LogsSize, m_Icons.Logs);

    m_IsLoaded = true;
}

void UI_Launcher::DrawToggleButton(const IconTexture& icon, UI_Tab& tab)
{
    bool active = tab.IsVisible();

    ImVec4 tint = active ?
        ImVec4(1.0f, 1.0f, 1.0f, 1.0f) :
        ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, active ?
        ImVec4(0.2f, 0.55f, 0.2f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

    if (ImGui::ImageButton("##icon", icon.ID(), ImVec2(48, 48),
        ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint))
    {
        tab.ToggleVisible();
    }

    ImGui::PopStyleColor();
}

void UI_Launcher::DrawDockSpace()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##dockspace", nullptr, flags);
    ImGui::PopStyleVar();

    ImGuiID id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void UI_Launcher::ToggleVisible()
{
    m_IsVisible = !m_IsVisible;
    s_IsVisible = m_IsVisible;
}

void UI_Launcher::ToggleAllTabs()
{
    bool anyVisible = false;
    for (auto& tab : m_Deps.Tabs)
    {
        if (tab->IsVisible()) 
        { 
            anyVisible = true; 
            break; 
        }
    }

    if (anyVisible)
    {
        m_PreviousTabState.clear();

        for (auto& tab : m_Deps.Tabs)
        {
            m_PreviousTabState.push_back(tab->IsVisible());

            if (tab->IsVisible())
            {
                tab->ToggleVisible();
            }
        }
    }
    else
    {
        if (m_PreviousTabState.size() == m_Deps.Tabs.size())
        {
            for (size_t i = 0; i < m_Deps.Tabs.size(); i++)
            {
                if (m_PreviousTabState[i] &&
                    !m_Deps.Tabs[i]->IsVisible())
                {
                    m_Deps.Tabs[i]->ToggleVisible();
                }
            }
        }
    }
}

const std::vector<UI_Tab*>& UI_Launcher::GetTabs() const
{
    return m_Deps.Tabs;
}

void UI_Launcher::ResetTabs()
{
    for (size_t i = 0; i < m_Deps.Tabs.size(); i++)
    {
        bool shouldBeVisible = !m_PreviousTabState.empty() ?
            m_PreviousTabState[i] : m_Deps.Tabs[i]->IsVisible();

        if (shouldBeVisible)
        {
            if (!m_Deps.Tabs[i]->IsVisible())
            {
                m_Deps.Tabs[i]->ToggleVisible();
            }

            m_Deps.Tabs[i]->RequestReset();
        }
    }
}