#include "pch.h"

// Header.
#include "CopyableField.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <vector>

void CopyableField::Draw(const char* label, const std::string& value, uint32_t ownerHandle)
{
	std::string uniqueId = std::to_string(ownerHandle) + "_" + label;

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

	std::vector<char> buffer(value.begin(), value.end());
	buffer.push_back('\0');

	ImGui::TextDisabled("%s", label);
	ImGui::SameLine();

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::InputText(
		("##copy_" + uniqueId).c_str(),
		buffer.data(),
		buffer.size(),
		ImGuiInputTextFlags_ReadOnly
	);

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Right-click to copy.");
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			ImGui::SetClipboardText(value.c_str());
			m_AnimateCopyLabel = uniqueId;
			m_AnimationStartTime = (float)ImGui::GetTime();
		}
	}

	if (m_AnimateCopyLabel == uniqueId)
	{
		float elapsed = (float)ImGui::GetTime() - m_AnimationStartTime;
		if (elapsed < m_AnimationDuration)
		{
			float alpha = 1.0f - (elapsed / m_AnimationDuration);
			ImGui::GetWindowDrawList()->AddRectFilled(
				ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
				ImColor(0.8f, 0.8f, 0.8f, alpha * 0.4f),
				ImGui::GetStyle().FrameRounding
			);
		}
		else { m_AnimateCopyLabel = ""; }
	}
}