#include "pch.h"
#include "Core/UI/CoreUI.h"
#include "Core/UI/MainInterface.h"
#include "Core/UI/Tabs/Interface/LogsTab.h"
//#include "Core/UI/Tabs/Optional/EventRegistryTab.h"
#include "Core/UI/Tabs/Domain/SettingsTab.h"
#include "Core/UI/Tabs/Domain/Tables/ObjectTableTab.h"
#include "Core/UI/Tabs/Domain/Tables/PlayerTableTab.h"
#include "Core/UI/Tabs/Domain/Graph/ObjectGraphTab.h"
#include "Core/UI/Tabs/Domain/Interactable/InteractableTab.h"
#include "Core/UI/Tabs/Domain/Physics/PhysicsDebugTab.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Domain/MainThread.h"
#include "External/imgui/imgui_internal.h"

void MainInterface::Draw()
{
	// Pre-render: Visibility and Input Management
	if (!g_pState->Infrastructure->Settings->IsMenuVisible())
	{
		ImGui::GetIO().ClearInputMouse();
		ImGui::GetIO().ClearInputKeys();
		return;
	}

	// Pre-render: Position reset if requested
	this->HandleWindowReset();

	// Default window settings
	ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);

	bool open = g_pState->Infrastructure->Settings->IsMenuVisible();
	bool isVisible = ImGui::Begin("Artemis - Control Panel", &open, ImGuiWindowFlags_None);

	if (!open)
	{
		g_pState->Infrastructure->Settings->SetMenuVisible(false);
	}

	if (isVisible)
	{
		this->DrawStatusBar();
		this->DrawTabs();
	}

	ImGui::End();
}


void MainInterface::HandleWindowReset()
{
	if (!g_pState->Infrastructure->Settings->MustResetMenu()) return;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 screenSize = viewport->Size;
	ImVec2 windowSize = ImVec2(1000, 600);

	ImGui::SetNextWindowPos(
		ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f),
		ImGuiCond_Always,
		ImVec2(0.5f, 0.5f)
	);

	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	g_pState->Infrastructure->Settings->SetForceMenuReset(false);
}

void MainInterface::DrawStatusBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 0));

	// Section: Engine Status
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Game Engine:");
	ImGui::SameLine();

	auto status = g_pState->Infrastructure->Lifecycle->GetEngineStatus();
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

	int fps = g_pState->Infrastructure->Render->GetFramerate();
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

void MainInterface::DrawTabs()
{
	static bool firstLaunch = true;

	if (!ImGui::BeginTabBar("MainTabs")) return;

	auto AddTab = [](const char* label, auto drawFn, bool forceOpen, const ImVec4* alertColor, bool disabled = false) {
		bool pushedColor = false;

		if (alertColor != nullptr)
		{
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration<float>(now - g_pSystem->Debug->GetLastAlertTime()).count();

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

	bool useAppData = g_pState->Infrastructure->Settings->ShouldUseAppData();

	AddTab("Object Table", []() { g_pUI->ObjectTable->Draw(); }, false, nullptr);
	AddTab("Player Table", []() { g_pUI->PlayerTable->Draw(); }, false, nullptr);
	AddTab("Object Graph", []() { g_pUI->ObjectGraph->Draw(); }, false, nullptr);
	AddTab("Interactable", []() { g_pUI->Interactable->Draw(); }, false, nullptr);
	AddTab("Physics Debug", []() { g_pUI->PhysicsDebug->Draw(); }, false, nullptr);
	AddTab("Settings", []() { g_pUI->Settings->Draw(); }, firstLaunch, nullptr);

	// Logs
	const ImVec4* activeAlert = nullptr;
	static ImVec4 colorError(1.0f, 0.33f, 0.33f, 1.0f);
	static ImVec4 colorWarning(1.0f, 0.79f, 0.23f, 1.0f);

	if (g_pSystem->Debug->HasUnreadError()) activeAlert = &colorError;
	else if (g_pSystem->Debug->HasUnreadWarning()) activeAlert = &colorWarning;

	AddTab("Logs", []() {
		g_pSystem->Debug->ClearUnreadStates();
		g_pUI->Logs->Draw();
	}, false, activeAlert);

	firstLaunch = false;
	ImGui::EndTabBar();
}