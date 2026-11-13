module;

#include "External/imgui/imgui.h"

module DevTools.Workbench.Settings.System;

import DevTools.Gui.Style.Type;
import DevTools.Gui.Widget.System;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Camera.Type;
import DevTools.Workbench.Hotkey.Type;
import DevTools.Workbench.Settings.Type;
import std;

namespace
{
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Options::Type::GroupInfo;
    using DevTools::Viewer::Options::Type::FlagInfo;
    using DevTools::Viewer::Options::Type::ScalarInfo;
    using DevTools::Viewer::Camera::Type::Mode;
    using DevTools::Viewer::Camera::Type::Follow;

    using DevTools::Workbench::Hotkey::Type::Binding;
    using DevTools::Workbench::Settings::Type::SettingsSection;

    using DevTools::Viewer::Options::Type::k_Groups;
    using DevTools::Viewer::Options::Type::k_Flags;
    using DevTools::Viewer::Options::Type::k_Scalars;
    using DevTools::Gui::Style::Type::StatusKind;
    using DevTools::Workbench::Settings::Type::k_SettingsSectionTitles;
    using DevTools::Gui::Style::Type::k_MinimumAlpha;
    using DevTools::Gui::Style::Type::k_MinimumScale;
    using DevTools::Gui::Style::Type::k_MaximumScale;
    using DevTools::Gui::Style::Type::k_AnimationDuration;
    using DevTools::Gui::Widget::System::DisabledScope;
    using DevTools::Gui::Widget::System::ResponsiveCardGuiService;
    using DevTools::Gui::Widget::System::SectionBox;
    using DevTools::Gui::Widget::System::StatusGuiService;

    constexpr std::array<const char*, 2> k_ModeLabels{ "Free camera", "Game camera" };
    constexpr std::array<const char*, 3> k_FollowLabels{ "Off", "First person", "Orbit" };

    // Share of a row the label takes before its item, and the narrowest an item may get (in font sizes) before it goes under its label.
    constexpr float k_LabelColumnFraction{ 0.4f };
    constexpr float k_MinimumItemEms{ 6.0f };

    // Seconds the save and load buttons show their confirmation.
    constexpr double k_FeedbackSeconds{ 2.0 };

    constexpr const char* k_DisablePopup{ "Confirm Disable AppData" };
    constexpr const char* k_DeletePopup{ "Delete AppData" };

    // Draws the label at the left of a row and sizes the next item to fill the rest of it, so the row follows the width of the window.
    // A row with too little room puts the item on the line below its label.
    // param tooltip: Shown while the label is hovered. Null for none.
    auto LabelBeforeItem(const char* label, const char* tooltip) -> void
    {
        const float startX{ ImGui::GetCursorPosX() };
        const float available{ ImGui::GetContentRegionAvail().x };
        const float column{ (std::max)(available * k_LabelColumnFraction, ImGui::CalcTextSize(label).x + ImGui::GetStyle().ItemSpacing.x) };

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        if (tooltip != nullptr) StatusGuiService::ShowTooltip(tooltip);

        if (available - column >= ImGui::GetFontSize() * k_MinimumItemEms) ImGui::SameLine(startX + column);
        ImGui::SetNextItemWidth(-(std::numeric_limits<float>::min)());
    }

    // Moves the cursor so the next buttons are centered.
    auto CenterNextButtons(float buttonWidth, int count) -> void
    {
        const float spacing{ ImGui::GetStyle().ItemSpacing.x };
        const float totalWidth{ (buttonWidth * count) + spacing * (count - 1) };

        const float offset{ (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f };
        if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
    }

    // Moves the cursor so the next text is centered in the column.
    auto CenterNextText(const char* text) -> void
    {
        const float columnWidth{ ImGui::GetColumnWidth() };
        const float textWidth{ ImGui::CalcTextSize(text).x };

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
    }
}

namespace DevTools::Workbench::Settings::System
{
    auto SettingsUIService::Draw() -> void
    {
        if (ImGui::BeginChild("SettingsScroll", ImVec2(0, 0), false))
        {
            if (this->BeginSection(SettingsSection::Data))
            {
                ImGui::Indent(10.0f);
                this->DrawData();
                ImGui::Unindent(10.0f);
                ImGui::Spacing();
            }

            if (this->BeginSection(SettingsSection::Viewer))
            {
                ImGui::Indent(10.0f);
                this->DrawViewerOptions();
                ImGui::Unindent(10.0f);
                ImGui::Spacing();
            }

            if (this->BeginSection(SettingsSection::Interface))
            {
                ImGui::Indent(10.0f);
                this->DrawInterface();
                ImGui::Unindent(10.0f);
                ImGui::Spacing();
            }

            if (this->BeginSection(SettingsSection::Hotkeys))
            {
                ImGui::Indent(10.0f);
                this->DrawHotkeysTable();
                ImGui::Unindent(10.0f);
                ImGui::Spacing();
            }

            if (this->BeginSection(SettingsSection::Folders))
            {
                ImGui::Indent(10.0f);
                this->DrawFolders();
                ImGui::Unindent(10.0f);
            }
        }

        ImGui::EndChild();
    }

    auto SettingsUIService::BeginSection(SettingsSection section) -> bool
    {
        const bool wasOpen{ m_SettingsUIStore.IsSectionOpen(section) };

        // The store owns the state, so it is saved with the preferences.
        ImGui::SetNextItemOpen(wasOpen, ImGuiCond_Always);
        const bool isOpen{ ImGui::CollapsingHeader(k_SettingsSectionTitles[static_cast<std::size_t>(section)]) };

        if (isOpen != wasOpen) m_SettingsUIStore.SetSectionOpen(section, isOpen);
        return isOpen;
    }

    auto SettingsUIService::DrawInterface() -> void
    {
        if (!m_IsScalePreviewInitialized)
        {
            m_UIScalePreview = m_SettingsStore.GetUIScale();
            m_IsScalePreviewInitialized = true;
        }

        SectionBox box{};

        LabelBeforeItem("Menu opacity", "Transparency of the Artemis windows.");

        float menuAlpha{ m_SettingsStore.GetMenuAlpha() };
        if (ImGui::SliderFloat("##GlobalOpacity", &menuAlpha, k_MinimumAlpha, 1.0f, "%.2f"))
        {
            m_SettingsStore.SetMenuAlpha((std::max)(menuAlpha, k_MinimumAlpha));
        }

        StatusGuiService::ShowTooltip("Transparency of the Artemis windows.");

        ImGui::Spacing();

        LabelBeforeItem("UI scale", "Size of the Artemis windows and their text. Applied when you release the slider.");

        ImGui::SliderFloat("##GlobalScale", &m_UIScalePreview, k_MinimumScale, k_MaximumScale, "%.2f");
        StatusGuiService::ShowTooltip("Size of the Artemis windows and their text. Applied when you release the slider.");

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            m_UIScalePreview = std::clamp(m_UIScalePreview, k_MinimumScale, k_MaximumScale);
            m_SettingsStore.SetUIScale(m_UIScalePreview);

            m_Log.Message("[SettingsUIService] INFO: Applied scale: {:.2f}.",
                m_UIScalePreview);
        }

        ImGui::Spacing();

        bool shouldFreezeMouse{ m_SettingsStore.IsMouseFreezeEnabled() };
        if (ImGui::Checkbox("Freeze mouse input", &shouldFreezeMouse))
        {
            m_SettingsStore.SetMouseFreezeEnabled(shouldFreezeMouse);
        }

        StatusGuiService::ShowTooltip("Blocks game camera movement while the menu is open.");

        ImGui::Spacing();

        const DisabledScope noStorage{ !m_SettingsStore.IsAppDataEnabled(), "Needs local storage, turn it on in Data." };

        bool shouldOpenOnStart{ m_SettingsStore.IsOpenUIOnStartEnabled() };
        if (ImGui::Checkbox("Open UI on MCC start", &shouldOpenOnStart))
        {
            m_SettingsStore.SetOpenUIOnStartEnabled(shouldOpenOnStart);
        }

        noStorage.Tooltip("Opens the Artemis control panel when MCC starts.");
    }

    auto SettingsUIService::DrawViewerOptions() -> void
    {
        for (const GroupInfo& group : k_Groups)
        {
            ImGuiTreeNodeFlags flags{ ImGuiTreeNodeFlags_Framed };
            if (group.IsOpenByDefault) flags |= ImGuiTreeNodeFlags_DefaultOpen;

            if (!ImGui::TreeNodeEx(group.Label, flags)) continue;

            this->DrawViewerGroup(group.Id);

            ImGui::TreePop();
            ImGui::Spacing();
        }

        if (ImGui::Button("Reset viewer options"))
        {
            m_OptionsStore.ResetToDefaults();
        }

        StatusGuiService::ShowTooltip("Sets every viewer option back to its default.");
    }

    auto SettingsUIService::DrawCameraControls() -> void
    {
        bool isActive{ m_CameraStore.IsActive() };
        if (ImGui::Checkbox("Viewer active", &isActive))
        {
            m_CameraStore.SetActive(isActive);
            if (!isActive) m_CameraStore.ResetKeys();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Shows or hides the Viewer. Same as the F4 hotkey.");
        }

        LabelBeforeItem("Mode", "Free camera: the Viewer takes the keyboard and the mouse and flies on its own.\n"
            "Game camera: the game keeps its controls and the Viewer sits on your view.");

        const int current{ static_cast<int>(m_CameraStore.GetMode()) };
        if (ImGui::BeginCombo("##ViewerMode", k_ModeLabels[current]))
        {
            for (int i{ 0 }; i < static_cast<int>(k_ModeLabels.size()); ++i)
            {
                const bool isSelected{ i == current };
                if (ImGui::Selectable(k_ModeLabels[i], isSelected))
                {
                    m_CameraStore.SetMode(static_cast<Mode>(i));
                }

                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }


        const bool isGameMode{ m_CameraStore.GetMode() == Mode::Game };
        if (isGameMode) ImGui::BeginDisabled();

        LabelBeforeItem("Follow selection", isGameMode ? "Needs the free camera." : "Off: flies on its own.\n"
            "First person: sits on the view of the selected object.\n"
            "Orbit: turns around the selected object (mouse rotates, wheel sets the distance).\n"
            "Same as the key left of the 1.");

        const int currentFollow{ static_cast<int>(m_CameraStore.GetFollow()) };
        if (ImGui::BeginCombo("##ViewerFollow", k_FollowLabels[currentFollow]))
        {
            for (int i{ 0 }; i < static_cast<int>(k_FollowLabels.size()); ++i)
            {
                const bool isSelected{ i == currentFollow };
                if (ImGui::Selectable(k_FollowLabels[i], isSelected))
                {
                    m_CameraStore.SetFollow(static_cast<Follow>(i));
                }

                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }


        if (isGameMode) ImGui::EndDisabled();

        bool isZoomActive{ m_CameraStore.IsZoomActive() };
        if (ImGui::Checkbox("Zoom active", &isZoomActive))
        {
            m_CameraStore.SetZoomActive(isZoomActive);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Narrows the field of view of the free camera. Same as the right click.");
        }
    }

    auto SettingsUIService::DrawViewerGroup(Group group) -> void
    {
        const bool areLabelsEnabled{ m_OptionsStore.IsEnabled(Flag::Labels) };

        SectionBox box{};

        if (group == Group::Camera) this->DrawCameraControls();

        for (const FlagInfo& info : k_Flags)
        {
            if (info.Group != group) continue;

            const bool isMaster{ info.Id == Flag::Labels };
            const bool isDisabled{ !isMaster && !areLabelsEnabled };

            if (isDisabled) ImGui::BeginDisabled();

            bool value{ m_OptionsStore.IsEnabled(info.Id) };
            if (ImGui::Checkbox(info.Label, &value))
            {
                m_OptionsStore.SetEnabled(info.Id, value);
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("%s", info.Tooltip);
            }

            if (isDisabled) ImGui::EndDisabled();
        }

        for (const ScalarInfo& info : k_Scalars)
        {
            if (info.Group != group) continue;

            LabelBeforeItem(info.Label, info.Tooltip);
            ImGui::PushID(info.Key);

            float value{ m_OptionsStore.GetScalar(info.Id) };
            if (ImGui::SliderFloat("##value", &value, info.Min, info.Max, info.Format))
            {
                m_OptionsStore.SetScalar(info.Id, value);
            }

            ImGui::PopID();
            }
    }

    auto SettingsUIService::DrawHotkeysTable() -> void
    {
        SectionBox box{};

        const ImGuiTableFlags flags{ ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_NoBordersInBody };

        if (!ImGui::BeginTable("HotkeysTable", 2, flags)) return;

        const ImVec4 headerColor{ ImGui::GetStyleColorVec4(ImGuiCol_TabActive) };

        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, headerColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, headerColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, headerColor);

        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthStretch, 0.4f);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        for (int i{ 0 }; i < 2; ++i)
        {
            ImGui::TableSetColumnIndex(i);

            const char* columnName{ ImGui::TableGetColumnName(i) };
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

    auto SettingsUIService::DrawData() -> void
    {
        bool shouldOpenDisablePopup{ false };
        bool shouldOpenDeletePopup{ false };

        {
            SectionBox box{};

            bool usesAppData{ m_SettingsStore.IsAppDataEnabled() };
            const std::string folder{ m_SettingsStore.GetAppDataDirectory() };

            StatusGuiService::DrawDot(usesAppData ? "Local storage: on" : "Local storage: off",
                usesAppData ? StatusKind::Active : StatusKind::Off,
                usesAppData && !folder.empty() ? folder.c_str() : "Nothing is saved to disk.");

            if (ImGui::Checkbox("Use local storage (AppData)", &usesAppData))
            {
                if (!usesAppData)
                {
                    shouldOpenDisablePopup = true;
                }
                else
                {
                    m_SettingsStore.SetAppDataEnabled(true);
                    m_SettingsService.CreateAppData();
                    m_SettingsService.SaveAppDataEnabled();
                }
            }

            StatusGuiService::ShowTooltip("Saves the preferences, replays and captures in the AppData folder.");

            const ImGuiStyle& style{ ImGui::GetStyle() };
            const float rightEdge{ ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x };
            const float buttonWidth{ (std::max)(ImGui::CalcTextSize("Save preferences").x, ImGui::CalcTextSize("Load preferences").x)
                + (style.FramePadding.x * 2.0f) };
            const bool justSaved{ ImGui::GetTime() < m_SaveFeedbackUntil };
            const bool justLoaded{ ImGui::GetTime() < m_LoadFeedbackUntil };

            {
                const DisabledScope noSave{ !usesAppData, "Needs local storage, turn it on above." };

                if (ImGui::Button(justSaved ? "Saved##SavePreferences" : "Save preferences##SavePreferences", ImVec2(buttonWidth, 0.0f)))
                {
                    m_PreferencesService.Save();
                    m_SaveFeedbackUntil = ImGui::GetTime() + k_FeedbackSeconds;
                }

                noSave.Tooltip("Writes the preferences to disk now.");
            }

            if (ResponsiveCardGuiService::FitsOnSameLine(buttonWidth, style.ItemSpacing.x, rightEdge)) ImGui::SameLine();

            {
                const DisabledScope noLoad{ !usesAppData, "Needs local storage, turn it on above." };

                if (ImGui::Button(justLoaded ? "Loaded##LoadPreferences" : "Load preferences##LoadPreferences", ImVec2(buttonWidth, 0.0f)))
                {
                    m_PreferencesService.Load();

                    // The scale slider shows the value in the store, which the file may have changed.
                    m_IsScalePreviewInitialized = false;
                    m_LoadFeedbackUntil = ImGui::GetTime() + k_FeedbackSeconds;
                }

                noLoad.Tooltip("Reads the preferences file again and applies it now.");
            }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

            if (ImGui::Button("Delete data")) shouldOpenDeletePopup = true;

            ImGui::PopStyleColor(2);
            StatusGuiService::ShowTooltip("Deletes the replays, timelines, event registry and preferences. Asks first.");
        }

        if (shouldOpenDisablePopup) ImGui::OpenPopup(k_DisablePopup);
        if (shouldOpenDeletePopup)  ImGui::OpenPopup(k_DeletePopup);

        this->DrawPersistencePopups();
    }

    auto SettingsUIService::DrawFolders() -> void
    {
        SectionBox box{ true };

        this->DrawPathField("Install", m_SettingsStore.GetBaseDirectory());
        this->DrawPathField("Log file", m_SettingsStore.GetLoggerPath());
        this->DrawPathField("Storage", m_SettingsStore.GetAppDataDirectory());
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

    auto SettingsUIService::DrawPathField(const char* label, const std::string& path) -> void
    {
        ImGui::TextDisabled("%s", label);
        ImGui::BeginGroup();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
        ImGui::SetNextItemWidth(-(std::numeric_limits<float>::min)());

        std::vector<char> buffer{ path.begin(), path.end() };
        buffer.push_back('\0');

        const std::string id{ std::string("##") + label };
        ImGui::InputText(id.c_str(), buffer.data(), buffer.size(), ImGuiInputTextFlags_ReadOnly);

        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Right-click to copy the path.");

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                ImGui::SetClipboardText(path.c_str());
                m_AnimatedPathLabel = label;
                m_AnimationStartTime = static_cast<float>(ImGui::GetTime());
            }
        }

        if (m_AnimatedPathLabel == label)
        {
            const float elapsed{ static_cast<float>(ImGui::GetTime()) - m_AnimationStartTime };

            if (elapsed < k_AnimationDuration)
            {
                const float alpha{ 1.0f - (elapsed / k_AnimationDuration) };

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

        constexpr float buttonWidth{ 120.0f };
        CenterNextButtons(buttonWidth, 2);

        if (ImGui::Button("Yes", ImVec2(buttonWidth, 0.0f)))
        {
            m_SettingsStore.SetAppDataEnabled(false);
            m_SettingsService.SaveAppDataEnabled();
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

        constexpr float buttonWidth{ 120.0f };
        CenterNextButtons(buttonWidth, 2);

        if (ImGui::Button("Yes", ImVec2(buttonWidth, 0.0f)))
        {
            m_SettingsService.DeleteAppData();
            m_SettingsStore.SetAppDataEnabled(false);
            m_SettingsService.SaveAppDataEnabled();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }
    }
}