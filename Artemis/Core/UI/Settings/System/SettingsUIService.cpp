module;

#include "External/imgui/imgui.h"

module UI.Settings.System;

import UI.Hotkey.Type;
import std;

namespace
{
	using Binding = UI::Hotkey::Type::Binding;

	constexpr float k_MinimumAlpha = 0.20f;
	constexpr float k_MinimumScale = 1.0f;
	constexpr float k_MaximumScale = 4.0f;

	constexpr const char* k_DisablePopup = "Confirm Disable AppData";
	constexpr const char* k_DeletePopup = "Delete AppData";

	class SectionBox
	{
	public:
		explicit SectionBox(bool useFullWidth = false) :
			m_UseFullWidth(useFullWidth),
			m_Min(ImGui::GetCursorScreenPos()),
			m_Width(ImGui::GetContentRegionAvail().x)
		{
			ImGui::BeginGroup();
			ImGui::Spacing();
			ImGui::Indent(10.0f);
			ImGui::Spacing();
		}

		~SectionBox()
		{
			ImGui::Spacing();
			ImGui::Unindent(10.0f);
			ImGui::Spacing();
			ImGui::EndGroup();

			const ImVec2 size = ImGui::GetItemRectSize();
			const float width = m_UseFullWidth ? m_Width : size.x + 20.0f;
			const ImVec2 max(m_Min.x + width, m_Min.y + size.y);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(m_Min, max, ImColor(255, 255, 255, 15), 5.0f);
			drawList->AddRect(m_Min, max, ImColor(255, 255, 255, 30), 5.0f);
		}

		SectionBox(const SectionBox&) = delete;
		SectionBox& operator=(const SectionBox&) = delete;

	private:
		bool m_UseFullWidth;
		ImVec2 m_Min;
		float m_Width;
	};

	auto CenterNextButtons(float buttonWidth, int count) -> void
	{
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float totalWidth = (buttonWidth * count) + spacing * (count - 1);

		const float offset = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
		if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
	}

	auto CenterNextText(const char* text) -> void
	{
		const float columnWidth = ImGui::GetColumnWidth();
		const float textWidth = ImGui::CalcTextSize(text).x;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
	}
}

namespace UI::Settings::System
{
	auto SettingsUIService::Draw() -> void
	{
		if (ImGui::BeginChild("SettingsScroll", ImVec2(0, 0), false))
		{
			if (ImGui::CollapsingHeader("User Preferences"))
			{
				ImGui::Indent(10.0f);
				this->DrawUserPreferences();
				ImGui::Unindent(10.0f);
				ImGui::Spacing();
			}

			if (ImGui::CollapsingHeader("Hotkeys", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10.0f);
				this->DrawHotkeysTable();
				ImGui::Unindent(10.0f);
				ImGui::Spacing();
			}

			if (ImGui::CollapsingHeader("Data Persistence"))
			{
				ImGui::Indent(10.0f);
				this->DrawDataPersistence();
				ImGui::Unindent(10.0f);
				ImGui::Spacing();
			}

			if (ImGui::CollapsingHeader("System Directories"))
			{
				ImGui::Indent(10.0f);
				this->DrawSystemDirectories();
				ImGui::Unindent(10.0f);
			}
		}

		ImGui::EndChild();
	}

	auto SettingsUIService::DrawUserPreferences() -> void
	{
		if (!m_IsScalePreviewInitialized)
		{
			m_UIScalePreview = m_SettingsStore.GetUIScale();
			m_IsScalePreviewInitialized = true;
		}

		SectionBox box;

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Menu Opacity");

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 205.0f);
		ImGui::PushItemWidth(200.0f);

		float menuAlpha = m_SettingsStore.GetMenuAlpha();
		if (ImGui::SliderFloat("##GlobalOpacity", &menuAlpha, k_MinimumAlpha, 1.0f, "%.2f"))
		{
			m_SettingsStore.SetMenuAlpha((std::max)(menuAlpha, k_MinimumAlpha));
		}

		ImGui::PopItemWidth();

		ImGui::Spacing();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("UI Scale");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 205.0f);

		ImGui::PushItemWidth(200.0f);
		ImGui::SliderFloat("##GlobalScale", &m_UIScalePreview, k_MinimumScale, k_MaximumScale, "%.2f");

		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			m_UIScalePreview = std::clamp(m_UIScalePreview, k_MinimumScale, k_MaximumScale);
			m_SettingsStore.SetUIScale(m_UIScalePreview);

			m_LogsService.Message("[SettingsUIService] INFO: Applied scale: {:.2f}",
				m_UIScalePreview);
		}

		ImGui::PopItemWidth();

		ImGui::Spacing();

		bool shouldFreezeMouse = m_SettingsStore.ShouldFreezeMouse();
		if (ImGui::Checkbox("Freeze Mouse Input", &shouldFreezeMouse))
		{
			m_SettingsStore.SetFreezeMouse(shouldFreezeMouse);
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Blocks game camera movement while the menu is open.");
		}

		ImGui::Spacing();

		const bool usesAppData = m_SettingsStore.ShouldUseAppData();
		if (!usesAppData) ImGui::BeginDisabled();

		bool shouldOpenOnStart = m_SettingsStore.ShouldOpenUIOnStart();
		if (ImGui::Checkbox("Open UI on MCC start", &shouldOpenOnStart))
		{
			m_SettingsStore.SetOpenUIOnStart(shouldOpenOnStart);
		}

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("%s", usesAppData ?
				"Defines if Artemis control panel is opened on MCC start." :
				"This feature uses Local Storage, go to Data Persistence to enable it.");
		}

		if (!usesAppData) ImGui::EndDisabled();
	}

	auto SettingsUIService::DrawHotkeysTable() -> void
	{
		SectionBox box;

		const float tableWidth = ImGui::GetContentRegionAvail().x - 10.0f;
		const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_NoBordersInBody;

		if (!ImGui::BeginTable("HotkeysTable", 2, flags, ImVec2(tableWidth, 0))) return;

		const ImVec4 headerColor = ImGui::GetStyleColorVec4(ImGuiCol_TabActive);

		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, headerColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, headerColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, headerColor);

		ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.6f);
		ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthStretch, 0.4f);

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int i = 0; i < 2; ++i)
		{
			ImGui::TableSetColumnIndex(i);

			const char* columnName = ImGui::TableGetColumnName(i);
			CenterNextText(columnName);
			ImGui::TableHeader(columnName);
		}

		for (const Binding& binding : m_HotkeyStore.GetBindings())
		{
			this->DrawHotkeyRow(binding.Label, binding.Keys, binding.Tooltip);
		}

		ImGui::PopStyleColor(3);
		ImGui::EndTable();
	}

	auto SettingsUIService::DrawDataPersistence() -> void
	{
		bool shouldOpenDisablePopup = false;
		bool shouldOpenDeletePopup = false;

		{
			SectionBox box;

			ImGui::BeginGroup();
			bool usesAppData = m_SettingsStore.ShouldUseAppData();
			ImGui::AlignTextToFramePadding();

			if (ImGui::Checkbox("Enable Local Storage (AppData)", &usesAppData))
			{
				if (!usesAppData)
				{
					shouldOpenDisablePopup = true;
				}
				else
				{
					m_SettingsStore.SetUseAppData(true);
					m_SettingsService.CreateAppData();
					m_SettingsService.SaveUseAppData();
				}
			}
			ImGui::EndGroup();

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 140.0f);

			ImGui::BeginGroup();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

			if (ImGui::Button("Delete Data", ImVec2(130, 0))) shouldOpenDeletePopup = true;

			ImGui::PopStyleColor(2);
			ImGui::EndGroup();
		}

		if (shouldOpenDisablePopup) ImGui::OpenPopup(k_DisablePopup);
		if (shouldOpenDeletePopup)  ImGui::OpenPopup(k_DeletePopup);

		this->DrawPersistencePopups();
	}

	auto SettingsUIService::DrawSystemDirectories() -> void
	{
		SectionBox box(true);

		this->DrawPathField("Base Installation", m_SettingsStore.GetBaseDirectory());
		this->DrawPathField("Log File Output", m_SettingsStore.GetLoggerPath());
		this->DrawPathField("Storage Folder", m_SettingsStore.GetAppDataDirectory());
	}

	auto SettingsUIService::DrawHotkeyRow(const char* label, const char* keys,
		const char* tooltip) -> void
	{
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		CenterNextText(label);
		ImGui::TextUnformatted(label);

		if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

		ImGui::TableNextColumn();
		CenterNextText(keys);
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", keys);
	}

	auto SettingsUIService::DrawPathField(const char* label, const std::string& path,
		float widthOffset) -> void
	{
		ImGui::TextDisabled("%s", label);
		ImGui::BeginGroup();

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - widthOffset);

		std::vector<char> buffer(path.begin(), path.end());
		buffer.push_back('\0');

		const std::string id = std::string("##") + label;
		ImGui::InputText(id.c_str(), buffer.data(), buffer.size(), ImGuiInputTextFlags_ReadOnly);

		ImGui::PopStyleColor();

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Right-click to copy path to clipboard.");

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				ImGui::SetClipboardText(path.c_str());
				m_AnimatedPathLabel = label;
				m_AnimationStartTime = static_cast<float>(ImGui::GetTime());
			}
		}

		if (m_AnimatedPathLabel == label)
		{
			const float elapsed = static_cast<float>(ImGui::GetTime()) - m_AnimationStartTime;

			if (elapsed < k_AnimationDuration)
			{
				const float alpha = 1.0f - (elapsed / k_AnimationDuration);

				ImGui::GetWindowDrawList()->AddRectFilled(
					ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
					ImColor(1.0f, 1.0f, 1.0f, alpha * 0.4f), ImGui::GetStyle().FrameRounding);
			}
			else
			{
				m_AnimatedPathLabel.clear();
			}
		}

		ImGui::EndGroup();
	}

	auto SettingsUIService::DrawPersistencePopups() -> void
	{
		if (ImGui::BeginPopupModal(k_DisablePopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			this->DrawConfirmDisableAppData();
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal(k_DeletePopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			this->DrawDeleteAllAppData();
			ImGui::EndPopup();
		}
	}

	auto SettingsUIService::DrawConfirmDisableAppData() -> void
	{
		ImGui::Text("Warning: Disabling local storage will prevent Artemis from saving"
			"\npreferences and using Replay Manager, Event Registry & Captures.");

		ImGui::Separator();

		constexpr float buttonWidth = 120.0f;
		CenterNextButtons(buttonWidth, 2);

		if (ImGui::Button("Yes", ImVec2(buttonWidth, 0.0f)))
		{
			m_SettingsStore.SetUseAppData(false);
			m_SettingsService.SaveUseAppData();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)))
		{
			ImGui::CloseCurrentPopup();
		}
	}

	auto SettingsUIService::DrawDeleteAllAppData() -> void
	{
		ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "WARNING: THIS ACTION CANNOT BE UNDONE");

		ImGui::Separator();

		ImGui::Text(
			"This will permanently delete:\n"
			" - All saved Replays.\n"
			" - All saved Timelines.\n"
			" - Custom Event Registry.\n"
			" - All user preferences.");

		ImGui::Separator();
		ImGui::Spacing();

		constexpr float buttonWidth = 120.0f;
		CenterNextButtons(buttonWidth, 2);

		if (ImGui::Button("Yes", ImVec2(buttonWidth, 0.0f)))
		{
			m_SettingsService.DeleteAppData();
			m_SettingsStore.SetUseAppData(false);
			m_SettingsService.SaveUseAppData();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)))
		{
			ImGui::CloseCurrentPopup();
		}
	}
}