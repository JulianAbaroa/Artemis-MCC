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
    using DevTools::Viewer::Director::State::DirectorStore;
    using DevTools::Viewer::Director::System::AddShot;
    using DevTools::Viewer::Director::System::AddToSequence;
    using DevTools::Viewer::Director::System::DefaultShotName;
    using DevTools::Viewer::Director::System::DeleteSequence;
    using DevTools::Viewer::Director::System::DescribeShotProgress;
    using DevTools::Viewer::Director::System::FindSequence;
    using DevTools::Viewer::Director::System::FindShotIndex;
    using DevTools::Viewer::Director::System::MoveInSequence;
    using DevTools::Viewer::Director::System::MoveShot;
    using DevTools::Viewer::Director::System::NewSequence;
    using DevTools::Viewer::Director::System::RemoveFromSequence;
    using DevTools::Viewer::Director::System::RemoveShot;
    using DevTools::Viewer::Director::System::RenameSequence;
    using DevTools::Viewer::Director::System::SanitizeName;
    using DevTools::Viewer::Director::System::ShotKindName;
    using DevTools::Viewer::Director::Type::DirectorFile;
    using DevTools::Viewer::Director::Type::DirectorFileInfo;
    using DevTools::Viewer::Director::Type::DirectorStatus;
    using DevTools::Viewer::Director::Type::Sequence;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotEnd;
    using DevTools::Viewer::Director::Type::ShotEndKind;
    using DevTools::Viewer::Director::Type::ShotKind;
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
    using DevTools::Viewer::Director::Type::k_DefaultShotRounds;
    using DevTools::Viewer::Director::Type::k_DefaultShotSeconds;

    constexpr std::array<const char*, 2> k_ModeLabels{ "Free camera", "Game camera" };
    constexpr std::array<const char*, 3> k_LevelLabels{ "Off", "Minimal", "Full" };

    constexpr std::size_t k_NameBufferSize{ 64 };
    constexpr float k_MaxShotSeconds{ 600.0f };
    constexpr int k_MaxShotRounds{ 20 };

    // Names of the drag and drop payloads: a shot of the library (its id and place) and a slot of the sequence list (its place).
    constexpr const char* k_ShotPayload{ "DIRECTOR_SHOT" };
    constexpr const char* k_SlotPayload{ "DIRECTOR_SLOT" };
    constexpr float k_MaxDegreesPerSecond{ 60.0f };

    // Narrowest a row may get (in font sizes) before the three level buttons become a combo.
    constexpr float k_MinimumSelectorEms{ 14.0f };

    // What a row of the library carries while it is dragged.
    struct DragShot
    {
        std::uint32_t Id{ 0 };
        std::size_t Index{ 0 };
    };

    // Applies the change to the shot with the id, when it still exists.
    auto EditShot(DirectorStore& store, std::uint32_t id, const std::function<void(Shot&)>& change) -> void
    {
        store.Edit([id, &change](DirectorFile& file) {
            const std::optional<std::size_t> index{ FindShotIndex(file, id) };
            if (!index) return false;

            change(file.Shots[*index]);
            return true;
        });
    }

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
        this->DrawFlag(Flag::AgentStrip);
        this->DrawFlag(Flag::RosterAgentsOnly);
        this->DrawFlag(Flag::DrawOverlay);
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
        const DirectorFileInfo info{ m_DirectorStore.GetFileInfo() };

        // Playing needs the Viewer: a stop is always possible.
        ImGui::BeginDisabled(!isActive && !status.IsPlaying);
        if (ImGui::Button(status.IsPlaying ? "Stop (F7)" : "Play (F7)")) m_DirectorStore.RequestTogglePlay();
        ImGui::EndDisabled();

        ImGui::SameLine();

        const std::string playing{ status.SequenceName.empty() ? std::string{ "all shots" } : status.SequenceName };

        if (!status.IsPlaying) ImGui::TextDisabled(isActive ? "stopped" : "activate the Viewer to play");
        else if (status.IsHolding) ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%s: waiting for a shot it can play", playing.c_str());
        else ImGui::Text("%s: shot %zu/%zu, %s", playing.c_str(), status.Index + 1, status.Count,
            DescribeShotProgress(status.End, static_cast<double>(status.ElapsedSeconds), status.RoundChanges).c_str());

        StatusGuiService::ShowTooltip("Plays the sequence of this map (or all its enabled shots when none is active) one shot after the other, in real time. "
            "Each shot is a pose of the free camera, an orbit or the first person view of the next Artemis bot, and ends by seconds, by rounds or never (hold). "
            "Stopping leaves the free camera where it was, or the game camera after a POV shot. Left and right arrows go to the previous and next shot while "
            "it plays. Deactivating the Viewer (F4) stops it.");

        ImGui::Text("Map: %s", info.MapKey.empty() ? "?" : info.MapKey.c_str());
        ImGui::TextDisabled("%s", info.Path.empty() ? "(no file)" : info.Path.c_str());
        ImGui::TextDisabled("%s", info.Message.c_str());

        this->DrawFlag(Flag::DirectorAutoPlay);
        this->DrawScalars(Group::Director, Layer::Count);

        ImGui::BeginDisabled(!isFree);
        if (ImGui::Button("Add fixed (free camera)")) m_DirectorStore.RequestAddFixed();
        StatusGuiService::ShowTooltip("Saves the pose of the free camera as a shot. Same as the Insert key. Only in Free mode.");

        ImGui::SameLine();
        if (ImGui::Button("Add orbit (around center)")) m_DirectorStore.RequestAddOrbit();
        StatusGuiService::ShowTooltip("Saves an orbit that starts where the free camera is and turns around the point it looks at, at the speed above. Only in Free mode.");
        ImGui::EndDisabled();

        const DirectorFile file{ m_DirectorStore.CopyFile() };

        ImGui::SameLine();
        if (ImGui::Button("Add POV"))
        {
            Shot shot{ .Kind = ShotKind::Pov, .Name = DefaultShotName(file.Shots, ShotKind::Pov) };
            shot.Id = m_DirectorStore.AllocateShotId();

            m_DirectorStore.Edit([&shot](DirectorFile& edited) {
                AddShot(edited, std::move(shot));
                return true;
            });
        }

        StatusGuiService::ShowTooltip("Adds a shot that shows the next Artemis bot in first person. Watches the next living bot in turn; the selected agent is not stored. "
            "It is skipped while there is no live bot.");

        this->DrawDirectorSequences(file, status);
        this->DrawDirectorLibrary(file, status);
    }

    auto ViewerUIService::DrawDirectorSequences(const DirectorFile& file, const DirectorStatus& status) -> void
    {
        ImGui::SeparatorText("Sequences");

        constexpr const char* k_NoSequence{ "(none: the whole library)" };
        const Sequence* active{ FindSequence(file, file.ActiveSequence) };

        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
        if (ImGui::BeginCombo("##sequence", active ? active->Name.c_str() : k_NoSequence))
        {
            if (ImGui::Selectable(k_NoSequence, active == nullptr))
            {
                m_DirectorStore.Edit([](DirectorFile& edited) {
                    edited.ActiveSequence.clear();
                    return true;
                });
            }

            for (const Sequence& sequence : file.Sequences)
            {
                if (!ImGui::Selectable(sequence.Name.c_str(), &sequence == active)) continue;

                m_DirectorStore.Edit([&sequence](DirectorFile& edited) {
                    edited.ActiveSequence = sequence.Name;
                    return true;
                });
            }

            ImGui::EndCombo();
        }

        StatusGuiService::ShowTooltip("The sequence that plays. With none, every enabled shot of the library plays in library order.");

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
        ImGui::InputText("##sequenceName", m_SequenceName.data(), m_SequenceName.size());

        const std::string typed{ m_SequenceName.data() };

        ImGui::SameLine();
        if (ImGui::Button("New"))
        {
            m_DirectorStore.Edit([&typed](DirectorFile& edited) {
                edited.ActiveSequence = NewSequence(edited, typed);
                return true;
            });
        }

        StatusGuiService::ShowTooltip("Adds an empty sequence with the name typed and makes it the active one.");

        ImGui::BeginDisabled(!active);
        ImGui::SameLine();
        if (ImGui::Button("Rename") && active)
        {
            m_DirectorStore.Edit([&](DirectorFile& edited) { return RenameSequence(edited, active->Name, typed).has_value(); });
        }

        ImGui::SameLine();
        if (ImGui::Button("Delete") && active)
        {
            m_DirectorStore.Edit([&](DirectorFile& edited) { return DeleteSequence(edited, active->Name); });
        }

        ImGui::EndDisabled();

        if (!active) return;

        // The shots the director plays are the enabled ones, so the shot that plays is found counting without the ones that are off.
        std::size_t playPosition{ 0 };
        const bool isThisPlaying{ status.IsPlaying && !status.IsHolding && status.SequenceName == active->Name };

        for (std::size_t slot{ 0 }; slot < active->ShotIds.size(); ++slot)
        {
            const std::uint32_t id{ active->ShotIds[slot] };
            const std::optional<std::size_t> index{ FindShotIndex(file, id) };
            const Shot* shot{ index ? &file.Shots[*index] : nullptr };

            const bool isCurrent{ isThisPlaying && shot && shot->IsEnabled && playPosition == status.Index };
            if (shot && shot->IsEnabled) ++playPosition;

            ImGui::PushID(static_cast<int>(slot));

            const std::string label{ shot ? std::format("{} {} {}{}", isCurrent ? ">" : "-", ShotKindName(shot->Kind), shot->Name, shot->IsEnabled ? "" : " (off)")
                : std::string{ "- (missing shot)" } };

            // note: The button goes FIRST and the row after it. The row is a Selectable as wide as the window, so a button placed after it (SameLine)
            // started at the right edge, outside the visible area, and the click never reached it. The name of the sequence is copied before the edit:
            // `active` points into the copy of the file of this frame.
            if (ImGui::SmallButton("x"))
            {
                const std::string sequenceName{ active->Name };

                m_DirectorStore.Edit([&sequenceName, slot](DirectorFile& edited) {
                    Sequence* sequence{ FindSequence(edited, sequenceName) };
                    return sequence && RemoveFromSequence(*sequence, slot);
                });
            }

            StatusGuiService::ShowTooltip("Takes the shot out of the sequence. It stays in the library.");

            ImGui::SameLine();
            ImGui::Selectable(label.c_str(), isCurrent);

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload(k_SlotPayload, &slot, sizeof(slot));
                ImGui::TextUnformatted(label.c_str());
                ImGui::EndDragDropSource();
            }

            this->AcceptIntoSequence(active->Name, slot);

            ImGui::PopID();
        }

        ImGui::Selectable(active->ShotIds.empty() ? "(drag shots from the library here)" : "(drop here to put it last)", false, ImGuiSelectableFlags_Disabled);
        this->AcceptIntoSequence(active->Name, active->ShotIds.size());
    }

    auto ViewerUIService::AcceptIntoSequence(const std::string& sequenceName, std::size_t slot) -> void
    {
        if (!ImGui::BeginDragDropTarget()) return;

        if (const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload(k_SlotPayload) })
        {
            const std::size_t from{ *static_cast<const std::size_t*>(payload->Data) };

            // note: A drop on the slot after the last one means "last", and the move is to the place the shot ends in.
            m_DirectorStore.Edit([&](DirectorFile& edited) {
                Sequence* sequence{ FindSequence(edited, sequenceName) };
                return sequence && MoveInSequence(*sequence, from, (std::min)(slot, sequence->ShotIds.size() - 1));
            });
        }
        else if (const ImGuiPayload* shotPayload{ ImGui::AcceptDragDropPayload(k_ShotPayload) })
        {
            const DragShot dragged{ *static_cast<const DragShot*>(shotPayload->Data) };

            m_DirectorStore.Edit([&](DirectorFile& edited) {
                Sequence* sequence{ FindSequence(edited, sequenceName) };
                return sequence && AddToSequence(*sequence, dragged.Id, slot);
            });
        }

        ImGui::EndDragDropTarget();
    }

    auto ViewerUIService::DrawDirectorLibrary(const DirectorFile& file, const DirectorStatus& status) -> void
    {
        ImGui::SeparatorText("Library");

        if (file.Shots.empty())
        {
            ImGui::TextDisabled("No shots for this map.");
            return;
        }

        const Sequence* active{ FindSequence(file, file.ActiveSequence) };

        // The shot that plays is marked here only when the whole library plays: a sequence marks its own list.
        const bool isLibraryPlaying{ status.IsPlaying && !status.IsHolding && active == nullptr };
        std::size_t playPosition{ 0 };

        constexpr ImGuiTableFlags k_Flags{ ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV };
        if (!ImGui::BeginTable("DirectorLibrary", 6, k_Flags)) return;

        ImGui::TableSetupColumn("##drag");
        ImGui::TableSetupColumn("On");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("End");
        ImGui::TableSetupColumn("##actions");
        ImGui::TableHeadersRow();

        for (std::size_t i{ 0 }; i < file.Shots.size(); ++i)
        {
            const Shot& shot{ file.Shots[i] };
            const std::uint32_t id{ shot.Id };

            const bool isCurrent{ isLibraryPlaying && shot.IsEnabled && playPosition == status.Index };
            if (shot.IsEnabled) ++playPosition;

            ImGui::PushID(static_cast<int>(id));
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Selectable(isCurrent ? ">>##drag" : "::##drag", isCurrent, ImGuiSelectableFlags_None, ImVec2(ImGui::GetFontSize() * 1.5f, 0.0f));

            if (ImGui::BeginDragDropSource())
            {
                const DragShot dragged{ .Id = id, .Index = i };
                ImGui::SetDragDropPayload(k_ShotPayload, &dragged, sizeof(dragged));
                ImGui::TextUnformatted(shot.Name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload(k_ShotPayload) })
                {
                    const DragShot dragged{ *static_cast<const DragShot*>(payload->Data) };
                    m_DirectorStore.Edit([&](DirectorFile& edited) { return MoveShot(edited.Shots, dragged.Index, i); });
                }

                ImGui::EndDragDropTarget();
            }

            StatusGuiService::ShowTooltip("Drag to reorder the library, or drop it on a sequence below to put it there.");

            ImGui::TableNextColumn();
            bool isEnabled{ shot.IsEnabled };
            if (ImGui::Checkbox("##on", &isEnabled)) EditShot(m_DirectorStore, id, [isEnabled](Shot& edited) { edited.IsEnabled = isEnabled; });
            StatusGuiService::ShowTooltip("A shot that is off stays in the library and in the sequences but is not played.");

            ImGui::TableNextColumn();
            std::array<char, k_NameBufferSize> name{};
            std::copy_n(shot.Name.begin(), (std::min)(shot.Name.size(), name.size() - 1), name.begin());
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.0f);
            if (ImGui::InputText("##name", name.data(), name.size())) EditShot(m_DirectorStore, id, [&name](Shot& edited) { edited.Name = SanitizeName(name.data()); });

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(std::string{ ShotKindName(shot.Kind) }.c_str());
            if (shot.Kind == ShotKind::Pov) StatusGuiService::ShowTooltip("Watches the next living bot in turn; the selected agent is not stored.");

            ImGui::TableNextColumn();
            this->DrawShotEnd(shot);

            ImGui::TableNextColumn();
            if (ImGui::SmallButton("+"))
            {
                m_DirectorStore.Edit([&](DirectorFile& edited) {
                    Sequence* sequence{ FindSequence(edited, edited.ActiveSequence) };
                    return sequence && AddToSequence(*sequence, id, sequence->ShotIds.size());
                });
            }

            StatusGuiService::ShowTooltip("Adds the shot at the end of the active sequence.");

            ImGui::SameLine();
            if (ImGui::SmallButton("Del")) m_DirectorStore.Edit([id](DirectorFile& edited) { return RemoveShot(edited, id); });
            StatusGuiService::ShowTooltip("Deletes the shot from the library and from every sequence.");

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    auto ViewerUIService::DrawShotEnd(const Shot& shot) -> void
    {
        constexpr std::array<const char*, 3> k_EndLabels{ "Seconds", "Rounds", "Hold" };
        const std::uint32_t id{ shot.Id };

        int kind{ static_cast<int>(shot.End.Kind) };
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0f);
        if (ImGui::Combo("##endKind", &kind, k_EndLabels.data(), static_cast<int>(k_EndLabels.size())))
        {
            const ShotEndKind newKind{ static_cast<ShotEndKind>(kind) };
            const float value{ newKind == ShotEndKind::Seconds ? k_DefaultShotSeconds : newKind == ShotEndKind::Rounds ? k_DefaultShotRounds : 0.0f };
            EditShot(m_DirectorStore, id, [newKind, value](Shot& edited) { edited.End = ShotEnd{ .Kind = newKind, .Value = value }; });
        }

        StatusGuiService::ShowTooltip("Seconds: ends after that much real time (rounds do not cut it). Rounds: ends at the N-th round change since it started "
            "(a change in its first 3 seconds does not count). Hold: never ends by itself, use the arrow keys.");

        if (shot.End.Kind == ShotEndKind::Seconds)
        {
            float seconds{ shot.End.Value };
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.5f);
            if (ImGui::DragFloat("##seconds", &seconds, 0.5f, 1.0f, k_MaxShotSeconds, "%.0f s", ImGuiSliderFlags_AlwaysClamp))
            {
                EditShot(m_DirectorStore, id, [seconds](Shot& edited) { edited.End.Value = seconds; });
            }
        }
        else if (shot.End.Kind == ShotEndKind::Rounds)
        {
            int rounds{ static_cast<int>(shot.End.Value) };
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.5f);
            if (ImGui::DragInt("##rounds", &rounds, 0.1f, 1, k_MaxShotRounds, "%d r", ImGuiSliderFlags_AlwaysClamp))
            {
                EditShot(m_DirectorStore, id, [rounds](Shot& edited) { edited.End.Value = static_cast<float>(rounds); });
            }
        }

        if (shot.Kind == ShotKind::Orbit)
        {
            float degreesPerSecond{ shot.DegreesPerSecond };
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.5f);
            if (ImGui::DragFloat("##turn", &degreesPerSecond, 0.1f, -k_MaxDegreesPerSecond, k_MaxDegreesPerSecond, "%.1f deg/s", ImGuiSliderFlags_AlwaysClamp))
            {
                EditShot(m_DirectorStore, id, [degreesPerSecond](Shot& edited) { edited.DegreesPerSecond = degreesPerSecond; });
            }
        }
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