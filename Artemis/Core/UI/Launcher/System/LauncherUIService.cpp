module;

#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"
#include "Resources/Icons/IconData.h"
#include <d3d11.h>

module UI.Launcher.System;

import UI.Icon.System;
import std;

namespace
{
	using Tab = UI::Launcher::Type::Tab;
	using IconLoader = UI::Icon::System::IconLoader;

	using UI::Launcher::Type::ToIndex;

	constexpr std::array<Tab, UI::Launcher::Type::k_TabCount> k_Tabs
	{
		Tab::ObjectTable,
		Tab::PlayerTable,
		Tab::Settings,
		Tab::MemoryScanner,
		Tab::Logs,
	};

	struct TabInfo
	{
		const char* ShortLabel;
		const char* Tooltip;
		const unsigned char* IconData;
		unsigned int IconSize;
	};

	auto GetTabInfo(Tab tab) -> TabInfo
	{
		namespace Data = ::Icons::Data;

		switch (tab)
		{
		case Tab::ObjectTable:   return { "Objects",  "Object Table",   Data::Objects,  Data::ObjectsSize };
		case Tab::PlayerTable:   return { "Players",  "Player Table",   Data::Players,  Data::PlayersSize };
		case Tab::Settings:      return { "Settings", "Settings",       Data::Settings, Data::SettingsSize };
		case Tab::MemoryScanner: return { "Scanner",  "Memory Scanner", Data::Scanner,  Data::ScannerSize };
		case Tab::Logs:          return { "Logs",     "Logs",           Data::Logs,     Data::LogsSize };
		default:                 return { "?",        "?",              nullptr,        0 };
		}
	}

	const ImVec2 k_TabDefaultSize{ 1360.0f, 800.0f };
}

namespace UI::Launcher::System
{
	auto LauncherUIService::OnInitialized() -> void
	{
		if (m_HasHandledOpenOnStart) return;
		m_HasHandledOpenOnStart = true;

		if (m_SettingsStore.ShouldOpenUIOnStart())
		{
			m_LauncherStore.SetVisible(true);
		}
	}

	auto LauncherUIService::ReleaseIcons() -> void
	{
		for (IconTexture& icon : m_Icons)
		{
			IconLoader::Release(icon);
		}

		m_AreIconsLoaded = false;
	}

	auto LauncherUIService::Update() -> bool
	{
		if (m_LauncherStore.ConsumeToggleAllTabs()) this->ToggleAllTabs();

		if (m_SettingsStore.MustResetMenu())
		{
			m_SettingsStore.SetForceMenuReset(false);
			this->ResetTabs();
		}

		const bool isMenuVisible = m_LauncherStore.IsVisible() ||
			m_LauncherStore.IsAnyTabVisible();

		m_SettingsStore.SetMenuVisible(isMenuVisible);
		return isMenuVisible;
	}

	auto LauncherUIService::Draw(const FrameContext& frame) -> void
	{
		const bool isVisible = m_LauncherStore.IsVisible();
		const bool hasAppeared = isVisible && !m_WasVisible;
		m_WasVisible = isVisible;

		if (!isVisible) return;

		if (!m_AreIconsLoaded) this->LoadIcons(frame);

		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always,
			ImVec2(0.5f, 0.5f));

		if (hasAppeared) ImGui::SetNextWindowFocus();

		const ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_AlwaysAutoResize;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

		if (ImGui::Begin("##launcher", nullptr, flags))
		{
			// Keep the launcher above the tabs without stealing keyboard focus.
			ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

			for (std::size_t i = 0; i < k_Tabs.size(); ++i)
			{
				if (i > 0) ImGui::SameLine();

				ImGui::PushID(static_cast<int>(i));
				this->DrawToggleButton(k_Tabs[i]);
				ImGui::PopID();
			}
		}

		ImGui::End();

		ImGui::PopStyleVar();
	}

	auto LauncherUIService::DrawDockSpace() -> void
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		const ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("##dockspace", nullptr, flags);
		ImGui::PopStyleVar();

		const ImGuiID id = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

		ImGui::End();
	}

	auto LauncherUIService::DrawTab(Tab tab, const char* title, const TabContent& content) -> void
	{
		if (!m_LauncherStore.IsTabVisible(tab)) return;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_FirstUseEver,
			ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(k_TabDefaultSize, ImGuiCond_FirstUseEver);

		if (m_LauncherStore.ConsumeTabReset(tab))
		{
			ImGuiWindow* window = ImGui::FindWindowByName(title);
			if (window != nullptr && window->DockId != 0)
			{
				ImGui::SetWindowDock(window, 0, ImGuiCond_Always);
			}

			ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always,
				ImVec2(0.5f, 0.5f));
		}

		ImGuiWindowFlags flags = ImGuiWindowFlags_None;
		if (m_SettingsStore.IsMenuLocked()) flags |= ImGuiWindowFlags_NoMove;

		bool isOpen = true;

		if (ImGui::Begin(title, &isOpen, flags))
		{
			content();
		}

		ImGui::End();

		if (!isOpen) m_LauncherStore.SetTabVisible(tab, false);
	}

	auto LauncherUIService::LoadIcons(const FrameContext& frame) -> void
	{
		for (Tab tab : k_Tabs)
		{
			const TabInfo info = GetTabInfo(tab);
			if (info.IconData == nullptr || info.IconSize == 0) continue;

			IconLoader::LoadIconFromMemory(frame.Device, info.IconData,
				info.IconSize, m_Icons[ToIndex(tab)]);
		}

		m_AreIconsLoaded = true;
	}

	auto LauncherUIService::DrawToggleButton(Tab tab) -> void
	{
		const TabInfo info = GetTabInfo(tab);
		const IconTexture& icon = m_Icons[ToIndex(tab)];
		const bool isActive = m_LauncherStore.IsTabVisible(tab);

		const float size = ImGui::GetFontSize() * 2.2f;

		const ImVec4 tint = isActive ?
			ImVec4(1.0f, 1.0f, 1.0f, 1.0f) :
			ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, isActive ?
			ImVec4(0.2f, 0.55f, 0.2f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

		bool isPressed = false;

		if (icon.IsValid())
		{
			isPressed = ImGui::ImageButton("##icon", icon.ID(), ImVec2(size, size),
				ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, tint);
			isPressed = ImGui::Button(info.ShortLabel, ImVec2(0.0f, size));
			ImGui::PopStyleColor();
		}

		ImGui::PopStyleColor();

		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", info.Tooltip);

		if (isPressed) m_LauncherStore.ToggleTab(tab);
	}

	auto LauncherUIService::ToggleAllTabs() -> void
	{
		if (m_LauncherStore.IsAnyTabVisible())
		{
			m_LauncherStore.SaveTabSnapshot();

			for (Tab tab : k_Tabs)
			{
				m_LauncherStore.SetTabVisible(tab, false);
			}

			return;
		}

		if (!m_LauncherStore.HasTabSnapshot()) return;

		for (Tab tab : k_Tabs)
		{
			if (m_LauncherStore.WasTabVisible(tab))
			{
				m_LauncherStore.SetTabVisible(tab, true);
			}
		}
	}

	auto LauncherUIService::ResetTabs() -> void
	{
		for (Tab tab : k_Tabs)
		{
			const bool shouldBeVisible = m_LauncherStore.IsTabVisible(tab) ||
				(m_LauncherStore.HasTabSnapshot() && m_LauncherStore.WasTabVisible(tab));

			if (!shouldBeVisible) continue;

			m_LauncherStore.SetTabVisible(tab, true);
			m_LauncherStore.RequestTabReset(tab);
		}
	}
}