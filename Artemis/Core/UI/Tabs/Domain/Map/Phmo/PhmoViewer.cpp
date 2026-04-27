#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Map/Phmo/MapPhmoState.h"
#include "Core/UI/Tabs/Domain/Map/TagRenderer.h"
#include "Core/UI/Tabs/Domain/Map/Phmo/PhmoViewer.h"
#include "Generated/Phmo/PhmoSchema.h"
#include "External/imgui/imgui.h"

std::vector<std::string> PhmoViewer::GetAvailableTags()
{
	return g_pState->Domain->MapPhmo->GetTagNames();
}

void PhmoViewer::DrawData(const std::string& tagName)
{
	const PhmoObject* phmo = g_pState->Domain->MapPhmo->GetPhmo(tagName);
	if (!phmo)
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Tag data not found.");
		return;
	}

	ImGui::Text("Inspector for: %s", tagName.c_str());
	ImGui::Separator();

	TagRenderer::Render(PhmoSchema::PhmoDataFields, &phmo->Data);
}