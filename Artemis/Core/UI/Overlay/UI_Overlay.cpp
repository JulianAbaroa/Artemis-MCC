#include "pch.h"

#include "UI_Overlay.h"

#include "Core/States/Lifecycle/State_Lifecycle.h"
#include "Core/States/Render/State_Render.h"

#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"

void UI_Overlay::Draw()
{
	if (!m_IsVisible) return;

	ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f),
		ImGuiCond_Always, ImVec2(0.0f, 0.0f));

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

	this->DrawFPS();

	ImGui::End();
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