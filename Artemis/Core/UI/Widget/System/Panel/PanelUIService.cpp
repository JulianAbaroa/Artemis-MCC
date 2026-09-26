module;

#include "External/imgui/imgui.h"

module UI.Widget.System;
import :Panel;

import UI.Format.System;
import std;

namespace
{
	using HexFormater = UI::Format::System::HexFormater;

	const ImVec4 k_SectionColor{ 0.8f, 0.8f, 0.8f, 1.0f };
	const ImVec4 k_FalseColor{ 0.4f, 0.4f, 0.4f, 1.0f };
}

namespace UI::Widget::System
{
	auto PanelUIService::DrawVec3(const char* label, const Vec3& value) -> void
	{
		ImGui::Text("%s %.3f  %.3f  %.3f", label, value.X, value.Y, value.Z);
	}

	auto PanelUIService::DrawBoolBadge(const char* label, bool value, const ImVec4& trueColor) -> void
	{
		ImGui::Text("%-10s", label);
		ImGui::SameLine();

		if (value) ImGui::TextColored(trueColor, "yes");
		else       ImGui::TextColored(k_FalseColor, "no");
	}

	auto PanelUIService::DrawSectionSeparator(const char* title) -> void
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (title) ImGui::TextColored(k_SectionColor, "%s", title);
	}

	auto PanelUIService::DrawHeader(const ImVec4& color, const char* title,
		const std::string& tagName, std::uint32_t handle) -> void
	{
		ImGui::TextColored(color, "%s", title);
		ImGui::Separator();
		ImGui::Text("%s", tagName.c_str());
		ImGui::Text("Handle: %s", HexFormater::Hex32(handle).c_str());
	}
}