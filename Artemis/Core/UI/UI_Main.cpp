#include "pch.h"

// Header.
#include "UI_Main.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"
#include "Core/States/Infrastructure/Engine/Render/State_Render.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"

// --- Systems ---

#include "Core/Systems/Interface/Debug/System_Debug.h"

// --- UI ---

#include "Core/UI/Core_UI.h"

#include "Domain/Object/UI_ObjectTable.h"
#include "Domain/Player/UI_PlayerTable.h"
#include "Domain/Graph/UI_ObjectGraph.h"
#include "Domain/Map/UI_Map.h"
#include "Domain/Interactable/UI_Interactable.h"
#include "Interface/UI_Logs.h"

#include "Infrastructure/Persistence/UI_Settings.h"
#include "Infrastructure/Memory/UI_MemoryScanner.h"

// ImGui.
#include "External/imgui/imgui_internal.h"

void UI_Main::Draw()
{
	// Pre-render: Visibility and Input Management
	if (!m_Deps.State_Settings.IsMenuVisible())
	{
		ImGui::GetIO().ClearInputMouse();
		ImGui::GetIO().ClearInputKeys();
		return;
	}

	// Pre-render: Position reset if requested
	this->HandleWindowReset();

	// Default window settings
	ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;

	if (m_Deps.State_Settings.IsMenuLocked())
	{
		windowFlags |= ImGuiWindowFlags_NoMove;
	}

	bool open = m_Deps.State_Settings.IsMenuVisible();

	bool isVisible = ImGui::Begin("Artemis - Control Panel", &open, windowFlags);

	if (!open)
	{
		m_Deps.State_Settings.SetMenuVisible(false);
	}

	if (isVisible)
	{
		this->DrawStatusBar();
		this->DrawTabs();
	}

	ImGui::End();
}


void UI_Main::HandleWindowReset()
{
	if (!m_Deps.State_Settings.MustResetMenu()) return;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 screenSize = viewport->Size;
	ImVec2 windowSize = ImVec2(1000, 600);

	ImGui::SetNextWindowPos(
		ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f),
		ImGuiCond_Always,
		ImVec2(0.5f, 0.5f)
	);

	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	m_Deps.State_Settings.SetForceMenuReset(false);
}

void UI_Main::DrawStatusBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 0));

	// Section: Engine Status
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Game Engine:");
	ImGui::SameLine();

	auto status = m_Deps.State_Lifecycle.GetEngineStatus();
	ImVec4 statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	const char* statusText = "UNKNOWN";

	switch (status) 
	{
	case EngineStatus::Waiting:
		statusText = "WAITING";
		statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
		break;

	case EngineStatus::Running:
		statusText = "RUNNING";
		statusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		break;

	case EngineStatus::Destroyed:
		statusText = "DESTROYED";
		statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		break;
	}

	ImGui::TextColored(statusColor, statusText);

	int fps = m_Deps.State_Render.GetFramerate();
	char fpsText[32];
	sprintf_s(fpsText, sizeof(fpsText), "%d FPS", fps);

	float textSize = ImGui::CalcTextSize(fpsText).x;
	float padding = ImGui::GetStyle().ItemSpacing.x;

	ImGui::SameLine(ImGui::GetWindowWidth() - textSize - padding - 20.0f);
	ImGui::AlignTextToFramePadding();

	ImVec4 fpsColor;
	if (fps >= 45.0f)
	{
		fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else if (fps >= 30.0f)
	{
		fpsColor = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
	}
	else
	{
		fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	ImGui::TextColored(fpsColor, fpsText);

	ImGui::ItemSize(ImVec2(0, 10.0f));
	ImGui::PopStyleVar();
	ImGui::Separator();
	ImGui::Spacing();
}

void UI_Main::DrawTabs()
{
	static bool firstLaunch = true;

	if (!ImGui::BeginTabBar("MainTabs")) return;

	auto AddTab = [this](const char* label, auto drawFn, bool forceOpen, const ImVec4* alertColor, bool disabled = false) {
		bool pushedColor = false;

		if (alertColor != nullptr)
		{
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration<float>(
				now - m_Deps.System_Debug.GetLastAlertTime()).count();

			ImVec4 finalColor = *alertColor;

			if (elapsed < 0.5f) 
			{
				float alpha = (sinf(elapsed * 18.84f - 1.57f) + 1.0f) * 0.5f;
				ImVec4 defaultTab = ImGui::GetStyleColorVec4(ImGuiCol_Tab);

				finalColor.x = ImLerp(defaultTab.x, finalColor.x, alpha);
				finalColor.y = ImLerp(defaultTab.y, finalColor.y, alpha);
				finalColor.z = ImLerp(defaultTab.z, finalColor.z, alpha);
			}

			ImGui::PushStyleColor(ImGuiCol_Tab, finalColor);
			ImGui::PushStyleColor(ImGuiCol_TabHovered, finalColor);
			ImGui::PushStyleColor(ImGuiCol_TabActive, finalColor);
			pushedColor = true;
		}

		if (disabled) ImGui::BeginDisabled(true);

		ImGuiTabItemFlags flags = forceOpen ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

		bool isOpen = ImGui::BeginTabItem(label, nullptr, flags);

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			if (disabled)
			{
				ImGui::SetTooltip("Local storage is required. Enable it in Settings -> Data Persistence.");
			}
		}

		if (isOpen) 
		{
			drawFn();
			ImGui::EndTabItem();
		}

		if (disabled) ImGui::EndDisabled();

		if (pushedColor) ImGui::PopStyleColor(3);
	};

	bool useAppData = m_Deps.State_Settings.ShouldUseAppData();

	

	AddTab("Object Table", [this]() { m_Deps.UI_ObjectTable.Draw(); }, false, nullptr);
	AddTab("Player Table", [this]() { m_Deps.UI_PlayerTable.Draw(); }, false, nullptr);
	AddTab("Object Graph", [this]() { m_Deps.UI_ObjectGraph.Draw(); }, false, nullptr);
	AddTab("Map", [this]() { m_Deps.UI_Map.Draw(); }, false, nullptr);
	AddTab("Interactable", [this]() { m_Deps.UI_Interactable.Draw(); }, false, nullptr);
	AddTab("Settings", [this]() { m_Deps.UI_Settings.Draw(); }, firstLaunch, nullptr);
	AddTab("Memory Scanner", [this]() { m_Deps.UI_MemoryScanner.Draw(); }, false, nullptr);

	// Logs
	const ImVec4* activeAlert = nullptr;
	static ImVec4 colorError(1.0f, 0.33f, 0.33f, 1.0f);
	static ImVec4 colorWarning(1.0f, 0.79f, 0.23f, 1.0f);

	if (m_Deps.System_Debug.HasUnreadError()) activeAlert = &colorError;
	else if (m_Deps.System_Debug.HasUnreadWarning()) activeAlert = &colorWarning;

	AddTab("Logs", [this]() {
		m_Deps.System_Debug.ClearUnreadStates();
		m_Deps.UI_Logs.Draw();
	}, false, activeAlert);

	firstLaunch = false;
	ImGui::EndTabBar();
}