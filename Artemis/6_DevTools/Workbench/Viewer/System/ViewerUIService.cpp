module;

#include "External/imgui/imgui.h"

module DevTools.Workbench.Viewer.System;

import Common.Engine.Type;
import Perception.Players.Type;
import DevTools.Gui.Style.Type;
import DevTools.Gui.Format.System;
import DevTools.Gui.Widget.System;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Director.Type;
import DevTools.Viewer.Director.System;
import std;

namespace
{
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using DevTools::Viewer::Camera::Type::Mode;
    using DevTools::Viewer::Director::System::DefaultShotName;
    using DevTools::Viewer::Director::System::ShotKindName;
    using DevTools::Viewer::Director::Type::DirectorFileInfo;
    using DevTools::Viewer::Director::Type::DirectorStatus;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Director::Type::ShotList;
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Options::Type::FlagInfo;
    using DevTools::Viewer::Options::Type::Group;
    using DevTools::Viewer::Options::Type::Layer;
    using DevTools::Viewer::Options::Type::LayerInfo;
    using DevTools::Viewer::Options::Type::Level;
    using DevTools::Viewer::Options::Type::ScalarInfo;
    using DevTools::Gui::Format::System::TeamFormater;
    using DevTools::Gui::Style::Type::StatusKind;
    using DevTools::Gui::Widget::System::DisabledScope;
    using DevTools::Gui::Widget::System::RowGuiService;
    using DevTools::Gui::Widget::System::StatusGuiService;

    using Common::Engine::Type::k_NoHandle;
    using DevTools::Viewer::Options::Type::IsFlagOn;
    using DevTools::Viewer::Options::Type::k_Flags;
    using DevTools::Viewer::Options::Type::k_LayerCount;
    using DevTools::Viewer::Options::Type::k_Layers;
    using DevTools::Viewer::Options::Type::k_Scalars;

    constexpr std::array<const char*, 2> k_ModeLabels{ "Free camera", "Game camera" };
    constexpr std::array<const char*, 3> k_LevelLabels{ "Off", "Minimal", "Full" };

    constexpr std::size_t k_NameBufferSize{ 64 };
    constexpr float k_MaxShotSeconds{ 600.0f };
    constexpr float k_MaxDegreesPerSecond{ 60.0f };

    // Narrowest a row may get (in font sizes) before the three level buttons become a combo.
    constexpr float k_MinimumSelectorEms{ 14.0f };

    // Appends the text to a list separated by commas.
    auto Append(std::string& list, const char* text) -> void
    {
        if (!list.empty()) list += ", ";
        list += text;
    }

    // return: The description of the layer followed by the options each level turns on, taken from the flag table.
    auto BuildLayerTooltip(const LayerInfo& layer) -> std::string
    {
        std::string minimal{};
        std::string added{};
        std::string dropped{};

        for (const FlagInfo& info : k_Flags)
        {
            if (info.Owner != layer.Id) continue;

            const bool isOnAtMinimal{ IsFlagOn(info, Level::Minimal) };
            const bool isOnAtFull{ IsFlagOn(info, Level::Full) };

            if (isOnAtMinimal) Append(minimal, info.Label);
            if (isOnAtFull && !isOnAtMinimal) Append(added, info.Label);
            if (isOnAtMinimal && !isOnAtFull) Append(dropped, info.Label);
        }

        std::string text{ layer.Tooltip };

        // note: A layer without flags (the agent card) describes its levels in its own tooltip.
        if (minimal.empty() && added.empty() && dropped.empty()) return text;

        text += "\nMinimal: " + minimal + ".";
        if (!added.empty()) text += "\nFull adds: " + added + ".";
        if (!dropped.empty()) text += "\nFull drops: " + dropped + ".";
        return text;
    }

    // return: Whether the layer has scalars of its own to show folded below its row.
    auto HasOwnScalars(Layer layer) -> bool
    {
        return std::ranges::any_of(k_Scalars, [layer](const ScalarInfo& info) { return info.Owner == layer; });
    }

    // return: The player the Viewer follows: the observed one, else the first local player. Null when the tick has none.
    auto FindFollowed(const Players* players, std::uint32_t observed) -> const Player*
    {
        if (!players) return nullptr;

        for (const Player& player : players->Rows)
        {
            if (observed != k_NoHandle ? player.Handle == observed : player.LocalIndex == 0) return &player;
        }

        return nullptr;
    }
}

namespace DevTools::Workbench::Viewer::System
{
    ViewerUIService::ViewerUIService(OptionsStore& optionsStore, CameraStore& cameraStore, const ObservedPlayerStore& observedStore, TickStore& tickStore,
        InspectorService& inspectorService, DirectorStore& directorStore) :
        m_OptionsStore(optionsStore), m_CameraStore(cameraStore), m_ObservedStore(observedStore), m_TickStore(tickStore),
        m_InspectorService(inspectorService), m_DirectorStore(directorStore)
    {
        for (const LayerInfo& info : k_Layers)
        {
            m_LayerTooltips[static_cast<std::size_t>(info.Id)] = BuildLayerTooltip(info);
        }
    }

    auto ViewerUIService::Draw() -> void
    {
        if (ImGui::BeginChild("ViewerScroll", ImVec2(0, 0), false))
        {
            this->DrawState();
            this->DrawPreset();
            this->DrawCamera();
            this->DrawLayers();
            this->DrawInspector();
            this->DrawDevelopment();

            ImGui::Spacing();

            if (ImGui::Button("Reset viewer options")) m_OptionsStore.ResetToDefaults();
            StatusGuiService::ShowTooltip("Sets every viewer option back to its default.");
        }

        ImGui::EndChild();
    }

    auto ViewerUIService::DrawState() -> void
    {
        const bool isActive{ m_CameraStore.IsActive() };
        StatusGuiService::DrawDot(isActive ? "Active" : "Inactive", isActive ? StatusKind::Active : StatusKind::Off);

        ImGui::SameLine();
        StatusGuiService::DrawChip(m_CameraStore.GetMode() == Mode::Free ? "Free" : "Game", StatusKind::Ok);

        const auto tick{ m_TickStore.Acquire() };
        const Players* players{ tick && tick->Players ? tick->Players.get() : nullptr };
        const Player* followed{ FindFollowed(players, m_ObservedStore.Get()) };

        ImGui::SameLine();

        if (followed)
        {
            ImGui::Text("%s (%s)", followed->Name.empty() ? "Player" : followed->Name.c_str(), TeamFormater::TeamToString(followed->Team));
        }
        else
        {
            ImGui::TextDisabled("No player");
        }
    }

    auto ViewerUIService::DrawPreset() -> void
    {
        RowGuiService::LabelBeforeItem("Preset", nullptr);

        if (!ImGui::BeginCombo("##ViewerPreset", "Debug")) return;

        ImGui::Selectable("Debug", true);

        for (const char* name : { "Showcase", "POV" })
        {
            DisabledScope disabled{ true, "Coming in VW.10" };
            ImGui::Selectable(name, false);
            disabled.Tooltip("");
        }

        ImGui::EndCombo();
    }

    auto ViewerUIService::DrawCamera() -> void
    {
        ImGui::SeparatorText("Camera");

        bool isActive{ m_CameraStore.IsActive() };
        if (ImGui::Checkbox("Viewer active", &isActive))
        {
            m_CameraStore.SetActive(isActive);
            if (!isActive) m_CameraStore.ResetKeys();
        }

        StatusGuiService::ShowTooltip("Shows or hides the Viewer. Same as the F4 hotkey.");

        RowGuiService::LabelBeforeItem("Mode", "Free camera: the Viewer takes the keyboard and the mouse and flies on its own, and the game draws from that camera.\n"
            "Game camera: the game keeps its controls and the Viewer draws from the camera of the game.");

        const int current{ static_cast<int>(m_CameraStore.GetMode()) };
        if (ImGui::BeginCombo("##ViewerMode", k_ModeLabels[current]))
        {
            for (int i{ 0 }; i < static_cast<int>(k_ModeLabels.size()); ++i)
            {
                const bool isSelected{ i == current };
                if (ImGui::Selectable(k_ModeLabels[i], isSelected)) m_CameraStore.SetMode(static_cast<Mode>(i));
                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        bool isZoomActive{ m_CameraStore.IsZoomActive() };
        if (ImGui::Checkbox("Zoom active", &isZoomActive)) m_CameraStore.SetZoomActive(isZoomActive);

        StatusGuiService::ShowTooltip("Narrows the field of view of the free camera. Same as the right click.");

        this->DrawFlag(Flag::HideGameHud);
        this->DrawFlag(Flag::HideFirstPerson);
        this->DrawFlag(Flag::WatchPov);
        this->DrawFlag(Flag::PossessBot);
        this->DrawScalars(Group::Camera, Layer::Count);

        this->DrawDirector();
    }

    auto ViewerUIService::DrawDirector() -> void
    {
        ImGui::SeparatorText("Director");

        const bool isActive{ m_CameraStore.IsActive() };
        const bool isFree{ m_CameraStore.GetMode() == Mode::Free };
        const DirectorStatus status{ m_DirectorStore.GetStatus() };
        const DirectorFileInfo file{ m_DirectorStore.GetFileInfo() };

        // Playing needs the Viewer: a stop is always possible.
        ImGui::BeginDisabled(!isActive && !status.IsPlaying);
        if (ImGui::Button(status.IsPlaying ? "Stop (F7)" : "Play (F7)")) m_DirectorStore.RequestTogglePlay();
        ImGui::EndDisabled();

        ImGui::SameLine();

        if (!status.IsPlaying) ImGui::TextDisabled(isActive ? "stopped" : "activate the Viewer to play");
        else if (status.IsHolding) ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "waiting for a shot it can play");
        else ImGui::Text("shot %zu/%zu, %.0f s", status.Index + 1, status.Count, status.ElapsedSeconds);

        StatusGuiService::ShowTooltip("Plays the shots of the map one after the other, in real time. Each shot is a pose of the free camera, an orbit or the "
            "first person view of the next Artemis bot. Stopping leaves the free camera where it was, or the game camera after a POV shot. Left and right "
            "arrows go to the previous and next shot while it plays. Deactivating the Viewer (F4) stops it.");

        ImGui::Text("Map: %s", file.MapKey.empty() ? "?" : file.MapKey.c_str());
        ImGui::TextDisabled("%s", file.Path.empty() ? "(no file)" : file.Path.c_str());
        ImGui::TextDisabled("%s", file.Message.c_str());

        this->DrawFlag(Flag::DirectorCutOnRound);
        this->DrawScalars(Group::Director, Layer::Count);

        ImGui::BeginDisabled(!isFree);
        if (ImGui::Button("Add fixed (free camera)")) m_DirectorStore.RequestAddFixed();
        StatusGuiService::ShowTooltip("Saves the pose of the free camera as a shot. Same as the Insert key. Only in Free mode.");

        ImGui::SameLine();
        if (ImGui::Button("Add orbit (around center)")) m_DirectorStore.RequestAddOrbit();
        StatusGuiService::ShowTooltip("Saves an orbit that starts where the free camera is and turns around the point it looks at, at the speed above. Only in Free mode.");
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Add POV"))
        {
            m_DirectorStore.AddShot(Shot{ .Kind = ShotKind::Pov, .Name = DefaultShotName(m_DirectorStore.CopyShots(), ShotKind::Pov) });
        }

        StatusGuiService::ShowTooltip("Adds a shot that shows the next Artemis bot in first person. It is skipped while there is no live bot.");

        const ShotList shots{ m_DirectorStore.CopyShots() };

        // The buttons only note what to do: the list does not change under the loop.
        std::optional<std::size_t> remove{};
        std::optional<std::size_t> moveUp{};
        std::optional<std::size_t> moveDown{};

        for (std::size_t i{ 0 }; i < shots.size(); ++i)
        {
            const Shot& shot{ shots[i] };
            ImGui::PushID(static_cast<int>(i));

            const bool isCurrent{ status.IsPlaying && !status.IsHolding && status.Index == i };
            ImGui::TextColored(isCurrent ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f), isCurrent ? ">" : "-");
            ImGui::SameLine();
            ImGui::Text("%-5s", std::string{ ShotKindName(shot.Kind) }.c_str());

            std::array<char, k_NameBufferSize> name{};
            std::copy_n(shot.Name.begin(), (std::min)(shot.Name.size(), name.size() - 1), name.begin());

            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.0f);
            if (ImGui::InputText("##name", name.data(), name.size())) m_DirectorStore.SetShotName(i, name.data());

            float seconds{ shot.Seconds };
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.5f);
            if (ImGui::DragFloat("##seconds", &seconds, 0.5f, 1.0f, k_MaxShotSeconds, "%.0f s", ImGuiSliderFlags_AlwaysClamp)) m_DirectorStore.SetShotSeconds(i, seconds);

            if (shot.Kind == ShotKind::Orbit)
            {
                float degreesPerSecond{ shot.DegreesPerSecond };
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.5f);
                if (ImGui::DragFloat("##turn", &degreesPerSecond, 0.1f, -k_MaxDegreesPerSecond, k_MaxDegreesPerSecond, "%.1f deg/s", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_DirectorStore.SetShotDegreesPerSecond(i, degreesPerSecond);
                }
            }

            ImGui::SameLine();
            ImGui::BeginDisabled(i == 0);
            if (ImGui::SmallButton("Up")) moveUp = i;
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::BeginDisabled(i + 1 >= shots.size());
            if (ImGui::SmallButton("Down")) moveDown = i;
            ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::SmallButton("Del")) remove = i;

            ImGui::PopID();
        }

        if (shots.empty()) ImGui::TextDisabled("No shots for this map.");

        if (moveUp) m_DirectorStore.MoveShot(*moveUp, -1);
        else if (moveDown) m_DirectorStore.MoveShot(*moveDown, 1);
        else if (remove) m_DirectorStore.RemoveShot(*remove);
    }

    auto ViewerUIService::DrawLayers() -> void
    {
        ImGui::SeparatorText("Layers");

        this->DrawFlag(Flag::Labels);

        if (ImGui::TreeNodeEx("Label settings", ImGuiTreeNodeFlags_None))
        {
            this->DrawScalars(Group::Layers, Layer::Count);
            ImGui::TreePop();
        }

        for (const LayerInfo& layer : k_Layers) this->DrawLayerRow(layer);
    }

    auto ViewerUIService::DrawLayerRow(const LayerInfo& layer) -> void
    {
        ImGui::PushID(layer.Key);

        RowGuiService::LabelBeforeItem(layer.Label, m_LayerTooltips[static_cast<std::size_t>(layer.Id)].c_str());
        this->DrawLevelSelector(layer.Id);

        if (HasOwnScalars(layer.Id))
        {
            ImGui::Indent();

            if (ImGui::TreeNodeEx("Settings", ImGuiTreeNodeFlags_None))
            {
                this->DrawScalars(Group::Layers, layer.Id);
                ImGui::TreePop();
            }

            ImGui::Unindent();
        }

        ImGui::PopID();
    }

    auto ViewerUIService::DrawLevelSelector(Layer layer) -> void
    {
        const int current{ static_cast<int>(m_OptionsStore.GetLevel(layer)) };
        const float available{ ImGui::GetContentRegionAvail().x };

        if (available < ImGui::GetFontSize() * k_MinimumSelectorEms)
        {
            if (ImGui::BeginCombo("##Level", k_LevelLabels[current]))
            {
                for (int i{ 0 }; i < static_cast<int>(k_LevelLabels.size()); ++i)
                {
                    const bool isSelected{ i == current };
                    if (ImGui::Selectable(k_LevelLabels[i], isSelected)) m_OptionsStore.SetLevel(layer, static_cast<Level>(i));
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            return;
        }

        const float spacing{ ImGui::GetStyle().ItemSpacing.x };
        const float width{ (available - spacing * (k_LevelLabels.size() - 1)) / static_cast<float>(k_LevelLabels.size()) };

        for (int i{ 0 }; i < static_cast<int>(k_LevelLabels.size()); ++i)
        {
            if (i > 0) ImGui::SameLine();

            const bool isSelected{ i == current };
            if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

            if (ImGui::Button(k_LevelLabels[i], ImVec2(width, 0.0f))) m_OptionsStore.SetLevel(layer, static_cast<Level>(i));

            if (isSelected) ImGui::PopStyleColor();
        }
    }

    auto ViewerUIService::DrawInspector() -> void
    {
        ImGui::Spacing();

        if (!ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) return;

        m_InspectorService.Draw();
    }

    auto ViewerUIService::DrawDevelopment() -> void
    {
        ImGui::Spacing();

        if (!ImGui::CollapsingHeader("Development")) return;

        this->DrawScalars(Group::Development, Layer::Count);
    }

    auto ViewerUIService::DrawFlag(Flag flag) -> void
    {
        const FlagInfo& info{ k_Flags[static_cast<std::size_t>(flag)] };

        bool value{ m_OptionsStore.IsEnabled(info.Id) };
        if (ImGui::Checkbox(info.Label, &value)) m_OptionsStore.SetEnabled(info.Id, value);

        StatusGuiService::ShowTooltip(info.Tooltip);
    }

    auto ViewerUIService::DrawScalars(Group group, Layer owner) -> void
    {
        for (const ScalarInfo& info : k_Scalars)
        {
            if (info.Group == group && info.Owner == owner) this->DrawScalar(info);
        }
    }

    auto ViewerUIService::DrawScalar(const ScalarInfo& info) -> void
    {
        RowGuiService::LabelBeforeItem(info.Label, info.Tooltip);
        ImGui::PushID(info.Key);

        float value{ m_OptionsStore.GetScalar(info.Id) };
        const ImGuiSliderFlags sliderFlags{ info.IsLogarithmic ? ImGuiSliderFlags_Logarithmic : ImGuiSliderFlags_None };
        if (ImGui::SliderFloat("##value", &value, info.Min, info.Max, info.Format, sliderFlags)) m_OptionsStore.SetScalar(info.Id, value);

        if (info.IsLogarithmic)
        {
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) m_OptionsStore.SetScalar(info.Id, info.Default);

            ImGui::TextDisabled("log2 %.2f", std::log2(m_OptionsStore.GetScalar(info.Id)));
        }

        ImGui::PopID();
    }
}