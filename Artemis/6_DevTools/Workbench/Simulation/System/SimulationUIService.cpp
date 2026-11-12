module;

#include "External/imgui/imgui.h"

module DevTools.Workbench.Simulation.System;

import Common.Engine.Type;
import Control.Action.Type;
import Engine.Base.Lifecycle.Type;
import Engine.Base.MatchSetup.Type;
import Engine.Control.Simulation.Type;
import Bridge.Transport.Type;
import Blam.Layout.Action.Type;
import Perception.Match.Type;
import DevTools.Gui.Style.Type;
import DevTools.Gui.Widget.System;
import std;

namespace
{
    using Engine::Control::Input::Type::LocalInputSource;
    using Engine::Control::Input::Type::PlayerAction;
    using Engine::Control::Input::Type::PlayerActions;
    using Control::Action::Type::ActionOverride;
    using Control::Action::Type::ClientRequests;
    using Engine::Control::Input::Type::ShadowCount;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Engine::Control::Simulation::Type::SessionSnapshot;
    using Engine::Control::Simulation::Type::SessionPlayer;
    using Engine::Control::Simulation::Type::Bot;
    using Engine::Control::Simulation::Type::FakeMachine;
    using Engine::Control::Simulation::Type::TeamRequest;
    using Engine::Base::MatchSetup::Type::MatchSetup;
    using Engine::Base::MatchSetup::Type::MatchSetupOrigin;
    using Engine::Base::MatchSetup::Type::Roster;
    using Engine::Base::MatchSetup::Type::ToText;
    using DevTools::Workbench::Simulation::Type::LocalPlayerAction;
    using DevTools::Workbench::Simulation::Type::SimulationSection;
    using DevTools::Gui::Style::Type::StatusKind;
    using DevTools::Gui::Widget::System::DisabledScope;
    using DevTools::Gui::Widget::System::ResponsiveCardGuiService;
    using DevTools::Gui::Widget::System::SectionBox;
    using DevTools::Gui::Widget::System::StatusGuiService;
    using Bridge::Transport::Type::BridgeStatus;
    using Contract::Bridge::Type::BridgeOwner;
    using Contract::Bridge::Type::BridgeMode;

    using Common::Engine::Type::k_MaxLocalUsers;
    using Engine::Base::MatchSetup::Type::k_MaxMachines;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxTeams;
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
    using Blam::Layout::Action::Type::Size::k_PlayerActionSize;
    using Blam::Layout::Action::Type::Offset::k_ActionButtonsOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionFacingOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionThrottleOffset;
    using Blam::Layout::Action::Type::k_UseLane;
    using Engine::Control::Input::Type::k_ShadowFieldCount;
    using Engine::Control::Input::Type::k_ShadowFieldNames;
    using Engine::Control::Input::Type::k_ShadowLogLimit;
    using DevTools::Workbench::Simulation::Type::k_LocalPlayerInputCount;
    using DevTools::Workbench::Simulation::Type::k_LocalPlayerInputLabels;
    using DevTools::Workbench::Simulation::Type::k_LocalPlayerActionCount;
    using DevTools::Workbench::Simulation::Type::k_LocalPlayerActionLabels;
    using DevTools::Workbench::Simulation::Type::k_RecordNameSize;
    using DevTools::Workbench::Simulation::Type::k_SimulationSectionTitles;
    using DevTools::Gui::Style::Type::k_StatusColors;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_TicksPerSecond;
    using Common::Engine::Type::k_HandleIndexMask;
    using DevTools::Gui::Style::Type::k_BytesPerRow;

    // Draws one radio button per local player action on a line.
    // return: The action the user clicked, or nothing when the selection did not change.
    auto DrawActionRadios(LocalPlayerAction current) -> std::optional<LocalPlayerAction>
    {
        std::optional<LocalPlayerAction> chosen{};

        for (std::size_t i{ 0 }; i < k_LocalPlayerActionCount; ++i)
        {
            if (i != 0) ImGui::SameLine();

            const auto mode{ static_cast<LocalPlayerAction>(i) };
            if (ImGui::RadioButton(k_LocalPlayerActionLabels[i], mode == current) && mode != current) chosen = mode;
        }

        return chosen;
    }

    // Width of the name field and of the list of the match config section.
    constexpr float k_MatchConfigFieldWidth{ 170.0f };

    // Names of the popups of the match config section.
    constexpr const char* k_RenameConfigPopup{ "Rename configuration" };
    constexpr const char* k_DeleteConfigPopup{ "Delete configuration" };

    // Width of the custom speed field, of the count buttons of the local players and of the input combo of their table.
    constexpr float k_CustomSpeedWidth{ 70.0f };
    constexpr float k_CountButtonWidth{ 28.0f };
    constexpr float k_InputComboWidth{ 150.0f };

    // Widths of the columns of the player tables.
    constexpr float k_NumberColumnWidth{ 28.0f };
    constexpr float k_NameColumnWidth{ 130.0f };
    constexpr float k_StateColumnWidth{ 110.0f };
    constexpr float k_KindColumnWidth{ 70.0f };

    // Size of the player actions window the first time it opens.
    constexpr ImVec2 k_PlayerActionsWindowSize{ 480.0f, 560.0f };

    // Lays controls on a line and starts the next line for the ones that do not fit in the width the line began with.
    class ControlFlow
    {
    public:
        ControlFlow() :
            m_RightEdge(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x)
        {
        }

        // Places the cursor for a control of the given width: on the same line when it fits, on the next one otherwise.
        auto Place(float width) -> void
        {
            if (!m_IsFirst && ResponsiveCardGuiService::FitsOnSameLine(width, ImGui::GetStyle().ItemSpacing.x, m_RightEdge)) ImGui::SameLine();
            m_IsFirst = false;
        }

        // Draws a button placed by Place. An active one is tinted like a pressed button.
        // return: Whether it was clicked.
        auto Button(const char* label, bool isActive = false) -> bool
        {
            this->Place(ImGui::CalcTextSize(label, nullptr, true).x + (ImGui::GetStyle().FramePadding.x * 2.0f));

            if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            const bool isClicked{ ImGui::Button(label) };
            if (isActive) ImGui::PopStyleColor();

            return isClicked;
        }

    private:
        float m_RightEdge{ 0.0f };
        bool m_IsFirst{ true };
    };

    // return: The counters of the bridge, one per line, for a tooltip.
    auto DescribeBridgeCounters(const BridgeStatus& status) -> std::string
    {
        const auto& counters{ status.Counters };
        const double waitMs{ counters.LockstepWaits > 0
            ? static_cast<double>(counters.LockstepWaitNs) / static_cast<double>(counters.LockstepWaits) / 1.0e6 : 0.0 };
        const double flattenMs{ counters.PublishedTicks > 0
            ? static_cast<double>(counters.FlattenNs) / static_cast<double>(counters.PublishedTicks) / 1.0e6 : 0.0 };

        return std::format("Shared memory Artemis.Bridge.{}\nN {}, d {}, agents 0x{:04X}\n"
            "Published ticks {} (flatten avg {:.3f} ms, max {:.3f})\nLate {}\nLockstep waits {} (avg {:.3f} ms, max {:.3f}), timeouts {}\n"
            "Handle mismatches {}, rejected {}, clock skips {}",
            status.Pid, static_cast<unsigned>(status.Control.DecisionPeriod), static_cast<unsigned>(status.Control.Latency),
            status.Control.AgentMask, counters.PublishedTicks, flattenMs, static_cast<double>(counters.FlattenMaxNs) / 1.0e6,
            counters.LateActions, counters.LockstepWaits, waitMs, static_cast<double>(counters.LockstepWaitMaxNs) / 1.0e6,
            counters.LockstepTimeouts, counters.HandleMismatches, counters.RejectedActions, counters.ClockSkips);
    }

    // Speeds tested in A8 (0.1 to 100). Above x5 the engine caps at 10 ticks per frame.
    constexpr float k_MinTimeScale{ 0.1f };
    constexpr float k_MaxTimeScale{ 100.0f };
    constexpr std::array<float, 7> k_TimeScalePresets{ 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f };

    // Virtual-key code of W. The engine button W (32) reads it through its key table.
    constexpr std::size_t k_ForwardKey{ 'W' };

    // Mouse movement delivered on every frame by the keyboard and mouse test, in mouse counts.
    constexpr float k_TestMouseDelta{ 1.0f };

    // Stick values of the gamepad test: full forward on the left stick and a steady turn on the right stick.
    constexpr std::int16_t k_StickForward{ 32767 };
    constexpr std::int16_t k_TestStickTurn{ 12000 };

    // Bits of the buttons of an action, and bits drawn per row.
    constexpr std::size_t k_ButtonBits{ 64 };
    constexpr std::size_t k_ButtonBitsPerRow{ 16 };

    // Colors of a button bit held this tick, held since the last clear, and of a byte that changed since the last clear.
    constexpr ImVec4 k_HeldColor{ 0.3f, 1.0f, 0.3f, 1.0f };
    constexpr ImVec4 k_SeenColor{ 1.0f, 0.8f, 0.2f, 1.0f };
    constexpr ImVec4 k_ChangedColor{ 1.0f, 0.4f, 0.3f, 1.0f };

    // Names of the teams of a team game, by team index.
    constexpr std::array<const char*, k_MaxTeams> k_TeamLabels{ "Red", "Blue", "Green", "Orange", "Purple", "Gold", "Brown", "Pink" };

    // return: The name of a team, or "-" outside 0..7.
    auto GetTeamLabel(std::int8_t team) -> const char*
    {
        if (team < 0 || static_cast<std::size_t>(team) >= k_TeamLabels.size()) return "-";
        return k_TeamLabels[static_cast<std::size_t>(team)];
    }

    // Radians to degrees.
    constexpr float k_RadiansToDegrees{ 57.2957795f };

    // Test action: full throttle forward and one degree of yaw per tick.
    constexpr float k_TestThrottle{ 1.0f };
    constexpr float k_TestYawPerTick{ 0.0174532925f };

    // return: The value of type T at the offset of the action.
    template<typename T>
    auto ReadAction(const PlayerAction& action, std::uintptr_t offset) -> T
    {
        T value{};
        std::memcpy(&value, action.Data.data() + offset, sizeof(T));
        return value;
    }

    // return: The player the engine maps to the local index, or null.
    auto FindLocalPlayer(const Players* players, std::size_t localIndex) -> const Player*
    {
        if (!players) return nullptr;

        for (const Player& player : players->Rows)
        {
            if (player.LocalIndex == localIndex) return &player;
        }

        return nullptr;
    }

    // return: The name as ASCII. Artemis names are ASCII.
    auto ToNarrow(std::wstring_view text) -> std::string
    {
        std::string out{};
        out.reserve(text.size());

        for (const wchar_t c : text)
        {
            out.push_back(static_cast<char>(c));
        }

        return out;
    }

    // return: The bytes as uppercase hex, two characters each.
    auto ToHex(std::span<const std::uint8_t> bytes) -> std::string
    {
        std::string out{};
        out.reserve(bytes.size() * 2);

        for (const std::uint8_t value : bytes)
        {
            out += std::format("{:02X}", value);
        }

        return out;
    }
}

namespace DevTools::Workbench::Simulation::System
{
    auto SimulationUIService::Draw(const std::function<void()>& drawTickDump, const std::function<void()>& drawDevelopmentTools) -> void
    {
        this->DrawStrip();

        ImGui::Spacing();

        if (this->BeginSection(SimulationSection::Match)) this->DrawMatchConfig();
        if (this->BeginSection(SimulationSection::Bots)) this->DrawBots();
        if (this->BeginSection(SimulationSection::TickDump)) drawTickDump();
        if (this->BeginSection(SimulationSection::LocalPlayers)) this->DrawLocalPlayers();
        if (this->BeginSection(SimulationSection::Development)) this->DrawDevelopment(drawDevelopmentTools);
    }

    auto SimulationUIService::DrawWindows() -> void
    {
        if (!m_SimulationUIStore.IsPlayerActionsOpen()) return;

        bool isOpen{ true };
        ImGui::SetNextWindowSize(k_PlayerActionsWindowSize, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Player actions", &isOpen))
        {
            this->DrawPlayerActions();

            ImGui::Spacing();
            ImGui::SeparatorText("Shadow check");

            this->DrawShadow();
        }

        ImGui::End();

        if (!isOpen) m_SimulationUIStore.SetPlayerActionsOpen(false);
    }

    auto SimulationUIService::BeginSection(SimulationSection section) -> bool
    {
        const bool wasOpen{ m_SimulationUIStore.IsSectionOpen(section) };

        // The store owns the state, so it survives the window and is saved with the preferences.
        ImGui::SetNextItemOpen(wasOpen, ImGuiCond_Always);
        const bool isOpen{ ImGui::CollapsingHeader(k_SimulationSectionTitles[static_cast<std::size_t>(section)]) };

        if (isOpen != wasOpen) m_SimulationUIStore.SetSectionOpen(section, isOpen);
        return isOpen;
    }

    auto SimulationUIService::DrawStrip() -> void
    {
        const SectionBox box{ true };

        this->DrawSpeedRow();
        this->DrawStatusRow();
    }

    auto SimulationUIService::DrawSpeedRow() -> void
    {
        const float requested{ m_LifecycleStore.GetRequestedTimeScale() };
        const bool isPaused{ m_LifecycleStore.IsPauseRequested() };

        // The client owns the speed and the pause while it owns the instance.
        BridgeStatus bridge{};
        m_BridgeService.CopyStatus(bridge);
        const bool isPythonOwned{ bridge.Control.Owner == BridgeOwner::Python };

        {
            const DisabledScope python{ isPythonOwned, "Owned by Python" };
            ControlFlow flow{};

            if (flow.Button(isPaused ? "Resume" : "Pause")) m_LifecycleStore.SetPauseRequested(!isPaused);
            python.Tooltip("Holds the logic ticks. The game keeps drawing frames without advancing.");

            if (flow.Button("Default", requested <= 0.0f)) m_LifecycleStore.SetRequestedTimeScale(0.0f);
            python.Tooltip("Stops writing the speed: the engine goes back to x1.");

            for (const float preset : k_TimeScalePresets)
            {
                const std::string label{ std::format("x{:g}", preset) };
                if (flow.Button(label.c_str(), requested == preset)) m_LifecycleStore.SetRequestedTimeScale(preset);
                python.Tooltip("Engine time scale. Above x5 the engine runs at most 10 ticks per frame.");
            }

            float custom{ m_SimulationUIStore.GetCustomTimeScale() };

            flow.Place(k_CustomSpeedWidth);
            ImGui::SetNextItemWidth(k_CustomSpeedWidth);
            if (ImGui::InputFloat("##CustomTimeScale", &custom, 0.0f, 0.0f, "%.2f"))
            {
                m_SimulationUIStore.SetCustomTimeScale(std::clamp(custom, k_MinTimeScale, k_MaxTimeScale));
            }
            python.Tooltip("Custom time scale, 0.1 to 100. Press Apply to use it.");

            if (flow.Button("Apply")) m_LifecycleStore.SetRequestedTimeScale(m_SimulationUIStore.GetCustomTimeScale());
            python.Tooltip("Applies the custom time scale. It is not saved: Artemis always starts without changing the speed.");
        }

        const float tickHz{ m_TelemetryStore.m_TickHz.load(std::memory_order_relaxed) };
        ImGui::Text("real x%.1f - %.0f t/s%s", tickHz / k_TicksPerSecond, tickHz, isPaused ? " (paused)" : "");
        StatusGuiService::ShowTooltip("Ticks per second the engine really runs. Above x5 it is capped at 10 ticks per frame.");
    }

    auto SimulationUIService::DrawStatusRow() -> void
    {
        BridgeStatus bridge{};
        m_BridgeService.CopyStatus(bridge);

        if (!bridge.IsOpen)                         StatusGuiService::DrawDot("Bridge", StatusKind::Error, "The shared memory is not open. See the log.");
        else if (bridge.Control.IsClientConnected)  StatusGuiService::DrawDot("Bridge", StatusKind::Active, "A Python client is connected.");
        else                                        StatusGuiService::DrawDot("Bridge", StatusKind::Off, "No Python client is connected.");

        // Recording is red (it costs disk and every tick), armed for the next load is amber.
        const bool isRecording{ m_TickDumpUIStore.IsRecording() };
        const bool isWaiting{ isRecording && m_TickDumpUIStore.IsWaitingForLoad() };

        ImGui::SameLine();
        if (isWaiting)        StatusGuiService::DrawDot("Dump", StatusKind::Pending, "Armed to record from the next load.");
        else if (isRecording) StatusGuiService::DrawDot("Dump", StatusKind::Error, "Recording ticks.");
        else                  StatusGuiService::DrawDot("Dump", StatusKind::Off, "Not recording.");

        ImGui::SameLine();
        if (!m_BenchUIStore.IsEnabled())             StatusGuiService::DrawDot("Bench", StatusKind::Off, "The test bench is off.");
        else if (m_BenchUIStore.IsScenarioRunning()) StatusGuiService::DrawDot("Bench", StatusKind::Active, "A bench scenario is running.");
        else                                         StatusGuiService::DrawDot("Bench", StatusKind::Off, "No bench scenario is running.");

        std::vector<Bot> bots{};
        m_BotStore.CopyBots(bots);

        ImGui::SameLine();
        ImGui::Text("Bots %zu/%zu", bots.size(), k_MaxPlayers);
        StatusGuiService::ShowTooltip("Bots Artemis has added to the match, out of the player slots of the engine.");
    }

    auto SimulationUIService::DrawDevelopment(const std::function<void()>& drawDevelopmentTools) -> void
    {
        ImGui::SeparatorText("Python bridge");
        this->DrawBridge();

        drawDevelopmentTools();

        ImGui::SeparatorText("Session");
        this->DrawSession();

        ImGui::Spacing();
        if (ImGui::Button("Player actions...")) m_SimulationUIStore.SetPlayerActionsOpen(true);
        StatusGuiService::ShowTooltip("Opens the window with the action of a local player in each tick and the shadow check.");
    }

    auto SimulationUIService::DrawMatchConfig() -> void
    {
        // Record: type a name, arm it and start a custom game from the MCC lobby, and its configuration is saved to the MatchConfigs
        // folder of the storage folder. Use: pick a recorded one, then start any custom game from the lobby or press Launch now. That
        // start plays the base map, map variant and game variant of the picked one, for one start only (D123).
        this->DrawMatchState();

        if (!m_SimulationUIStore.IsConfigListLoaded()) this->RefreshConfigs();

        std::size_t selected{ m_SimulationUIStore.GetSelectedMatchConfig() };

        if (selected >= m_SimulationUIStore.GetMatchConfigNames().size())
        {
            selected = 0;
            m_SimulationUIStore.SetSelectedMatchConfig(selected);
        }

        ControlFlow flow{};
        flow.Place(k_MatchConfigFieldWidth);
        ImGui::SetNextItemWidth(k_MatchConfigFieldWidth);

        const std::vector<std::string>& listed{ m_SimulationUIStore.GetMatchConfigNames() };
        const std::string preview{ listed.empty() ? std::string{ "No recorded configuration" } : listed[selected] };
        const bool isComboOpen{ ImGui::BeginCombo("##Recorded", preview.c_str()) };

        // The folder is read each time the list opens, so a configuration recorded meanwhile shows up without a button.
        if (isComboOpen && !m_SimulationUIStore.IsConfigComboOpen()) this->RefreshConfigs();
        m_SimulationUIStore.SetConfigComboOpen(isComboOpen);

        if (isComboOpen)
        {
            const std::vector<std::string>& names{ m_SimulationUIStore.GetMatchConfigNames() };

            for (std::size_t index{ 0 }; index < names.size(); ++index)
            {
                if (ImGui::Selectable(names[index].c_str(), index == m_SimulationUIStore.GetSelectedMatchConfig()))
                {
                    m_SimulationUIStore.SetSelectedMatchConfig(index);
                }
            }

            ImGui::EndCombo();
        }
        StatusGuiService::ShowTooltip("Recorded match configuration. The list is read from the storage folder each time it opens.");

        const std::vector<std::string>& names{ m_SimulationUIStore.GetMatchConfigNames() };
        selected = (std::min)(m_SimulationUIStore.GetSelectedMatchConfig(), names.empty() ? 0 : names.size() - 1);
        const std::string name{ names.empty() ? std::string{} : names[selected] };

        {
            const DisabledScope none{ names.empty(), "No recorded configuration." };

            if (flow.Button("Use for next start")) m_MatchConfigService.LoadPending(name);
            none.Tooltip("The next start of MCC plays the base map, map variant and game variant of the picked configuration, once.");
        }

        if (m_MatchConfigStore.IsLaunchRequested())
        {
            if (flow.Button("Cancel launch")) m_MatchConfigService.CancelLaunch();
            StatusGuiService::ShowTooltip("Withdraws the launch: MCC has not taken the request yet.");
        }
        else
        {
            // note: The lobby flag is set by the lifecycle hook, which looks for the lobby screen every 15 frames while no launch is
            // pending, so this button follows the real lobby with a delay of about 250 ms at 60 fps.
            const DisabledScope outsideLobby{ !m_MatchCycleStore.IsLobbyOpen(), "Only in the MCC lobby" };

            if (flow.Button("Launch now")) m_MatchConfigService.RequestLaunch();
            outsideLobby.Tooltip("Presses the START of the custom games lobby of MCC: MCC counts down and launches."
                " The pending configuration, if any, is transplanted into that start.");
        }

        this->DrawConfigEdit(name);

        const auto pending{ m_MatchConfigStore.GetPending() };
        const std::string history{ std::format("Last applied: {}\nLast recorded: {}\nLast launch: {}",
            m_MatchConfigStore.GetLastApplied().empty() ? std::string{ "-" } : m_MatchConfigStore.GetLastApplied(),
            m_MatchConfigStore.GetLastRecorded().empty() ? std::string{ "-" } : m_MatchConfigStore.GetLastRecorded(),
            m_MatchConfigStore.GetLastLaunch().empty() ? std::string{ "-" } : m_MatchConfigStore.GetLastLaunch()) };

        if (pending)
        {
            const std::string chip{ std::format("Next start: {}", pending->Name) };
            StatusGuiService::DrawChip(chip.c_str(), StatusKind::Pending, history.c_str());

            ImGui::SameLine();
            if (ImGui::SmallButton("x##ClearPending")) m_MatchConfigService.ClearPending();
            StatusGuiService::ShowTooltip("Clears it: the next start is the match of MCC unchanged.");
        }
        else
        {
            StatusGuiService::DrawChip("Next start: unchanged", StatusKind::Off, history.c_str());
        }

        std::array<char, k_RecordNameSize>& recordName{ m_SimulationUIStore.GetRecordNameField() };
        const std::string nameProblem{ MatchConfigService::GetNameProblem(recordName.data()) };
        const bool isTyped{ recordName[0] != '\0' };

        // An invalid name is marked in red with its reason, so it does not only show in the log.
        const bool isMarked{ isTyped && !nameProblem.empty() };
        if (isMarked) ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.45f, 0.12f, 0.12f, 1.0f });
        ImGui::SetNextItemWidth(k_MatchConfigFieldWidth);
        ImGui::InputTextWithHint("##RecordName", "Name to record", recordName.data(), recordName.size());
        if (isMarked) ImGui::PopStyleColor();
        StatusGuiService::ShowTooltip("Name to record the next start under.");

        ImGui::SameLine();
        {
            const DisabledScope invalid{ !nameProblem.empty(), nameProblem.c_str() };

            if (ImGui::Button("Record next start")) m_MatchConfigService.ArmRecord(recordName.data());
            invalid.Tooltip("Saves the configuration of the next start of MCC under this name, before any transplant."
                " A recorded configuration with the same name is replaced.");
        }

        if (isMarked) ImGui::TextColored(k_StatusColors[static_cast<std::size_t>(StatusKind::Error)], "%s", nameProblem.c_str());

        const std::string armedName{ m_MatchConfigStore.GetRecordName() };

        if (!armedName.empty())
        {
            const std::string chip{ std::format("Armed: {}", armedName) };
            StatusGuiService::DrawChip(chip.c_str(), StatusKind::Pending, "The next start of MCC is saved under this name.");

            ImGui::SameLine();
            if (ImGui::SmallButton("Cancel##Record")) m_MatchConfigService.CancelRecord();
            StatusGuiService::ShowTooltip("Disarms the record.");
        }
    }

    auto SimulationUIService::DrawMatchState() -> void
    {
        using Engine::Base::Lifecycle::Type::Status;

        const Status status{ m_LifecycleStore.GetStatus() };

        if (status == Status::Running)
        {
            std::string text{ "In match" };

            const std::string applied{ m_MatchConfigStore.GetLastApplied() };
            if (!applied.empty()) text += ": " + applied;

            const auto tick{ m_TickStore.Acquire() };

            if (tick && tick->Match)
            {
                const auto round{ tick->Match->RoundIndex + 1 };
                text += tick->Match->RoundLimit > 0 ? std::format(", round {}/{}", round, tick->Match->RoundLimit)
                                                    : std::format(", round {}", round);
            }

            StatusGuiService::DrawChip(text.c_str(), StatusKind::Active, "A match is running. The configuration is the last one applied.");
        }
        else if (status == Status::Initialized)
        {
            StatusGuiService::DrawChip("Loading", StatusKind::Pending, "The engine is starting a match.");
        }
        else if (m_MatchCycleStore.IsLobbyOpen())
        {
            StatusGuiService::DrawChip("Lobby", StatusKind::Ok, "MCC shows the custom games lobby.");
        }
        else
        {
            StatusGuiService::DrawChip("No match", StatusKind::Off, "MCC is not in a match or in the custom games lobby.");
        }
    }

    auto SimulationUIService::DrawConfigEdit(const std::string& name) -> void
    {
        {
            const auto pending{ m_MatchConfigStore.GetPending() };
            const bool isPending{ pending && pending->Name == name };

            const DisabledScope scope{ name.empty() || isPending,
                name.empty() ? "No recorded configuration." : "It is the pending configuration. Clear it first." };
            ControlFlow flow{};

            if (flow.Button("Rename..."))
            {
                std::array<char, k_RecordNameSize>& field{ m_SimulationUIStore.GetRenameField() };
                std::snprintf(field.data(), field.size(), "%s", name.c_str());

                m_SimulationUIStore.SetConfigEditError({});
                ImGui::OpenPopup(k_RenameConfigPopup);
            }
            scope.Tooltip("Renames the picked configuration.");

            if (flow.Button("Delete..."))
            {
                m_SimulationUIStore.SetConfigEditError({});
                ImGui::OpenPopup(k_DeleteConfigPopup);
            }
            scope.Tooltip("Moves the picked configuration to the _deleted folder of MatchConfigs.");
        }

        const ImVec4 errorColor{ k_StatusColors[static_cast<std::size_t>(StatusKind::Error)] };

        if (ImGui::BeginPopupModal(k_RenameConfigPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Rename \"%s\" to:", name.c_str());

            std::array<char, k_RecordNameSize>& field{ m_SimulationUIStore.GetRenameField() };
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::SetNextItemWidth(k_MatchConfigFieldWidth);
            ImGui::InputText("##NewName", field.data(), field.size());

            const std::string problem{ MatchConfigService::GetNameProblem(field.data()) };
            if (field[0] != '\0' && !problem.empty()) ImGui::TextColored(errorColor, "%s", problem.c_str());

            {
                const DisabledScope invalid{ !problem.empty(), problem.c_str() };

                if (ImGui::Button("Rename"))
                {
                    const std::string error{ m_MatchConfigService.RenameConfig(name, field.data()) };
                    m_SimulationUIStore.SetConfigEditError(error);

                    if (error.empty())
                    {
                        this->RefreshConfigs(field.data());
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel##Rename")) ImGui::CloseCurrentPopup();

            const std::string& error{ m_SimulationUIStore.GetConfigEditError() };
            if (!error.empty()) ImGui::TextColored(errorColor, "%s", error.c_str());

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal(k_DeleteConfigPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Delete \"%s\"?", name.c_str());
            ImGui::TextUnformatted("The file moves to MatchConfigs/_deleted. The lists in Data/Training and the\n"
                "entries of Maps.json that name it will stop loading.");

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.65f, 0.15f, 0.15f, 1.0f });
            const bool isConfirmed{ ImGui::Button("Delete") };
            ImGui::PopStyleColor();

            if (isConfirmed)
            {
                const std::string error{ m_MatchConfigService.DeleteConfig(name) };
                m_SimulationUIStore.SetConfigEditError(error);

                if (error.empty())
                {
                    this->RefreshConfigs();
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel##Delete")) ImGui::CloseCurrentPopup();

            const std::string& error{ m_SimulationUIStore.GetConfigEditError() };
            if (!error.empty()) ImGui::TextColored(errorColor, "%s", error.c_str());

            ImGui::EndPopup();
        }
    }

    auto SimulationUIService::RefreshConfigs(std::string_view prefer) -> void
    {
        const std::vector<std::string>& before{ m_SimulationUIStore.GetMatchConfigNames() };
        const std::size_t picked{ m_SimulationUIStore.GetSelectedMatchConfig() };
        const std::string keep{ !prefer.empty() ? std::string{ prefer } : picked < before.size() ? before[picked] : std::string{} };

        std::vector<std::string> names{ m_MatchConfigService.ListConfigs() };
        const auto found{ std::ranges::find(names, keep) };
        const std::size_t index{ found == names.end() ? 0 : static_cast<std::size_t>(found - names.begin()) };

        m_SimulationUIStore.SetMatchConfigNames(std::move(names));
        m_SimulationUIStore.SetSelectedMatchConfig(index);
        m_SimulationUIStore.SetConfigListLoaded(true);
    }

    auto SimulationUIService::DrawBridge() -> void
    {
        BridgeStatus status{};
        m_BridgeService.CopyStatus(status);

        if (!status.IsOpen)
        {
            StatusGuiService::DrawDot("Off", StatusKind::Error, "The shared memory is not open. See the log.");
            return;
        }

        const std::string line{ std::format("client {}, {}, owner {}", status.Control.IsClientConnected ? "connected" : "none",
            status.Control.Mode == BridgeMode::Lockstep ? "Lockstep" : "Free", status.Control.Owner == BridgeOwner::Python ? "Python" : "UI") };
        const std::string counters{ DescribeBridgeCounters(status) };

        StatusGuiService::DrawDot(line.c_str(), status.Control.IsClientConnected ? StatusKind::Active : StatusKind::Off, counters.c_str());
    }

    auto SimulationUIService::DrawLocalPlayers() -> void
    {
        // Split-screen users Artemis adds next to yours. The count applies when the next match starts.
        const std::size_t previousCount{ m_LocalUserStore.GetRequestedCount() };

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("At next start");
        StatusGuiService::ShowTooltip("Split-screen users Artemis adds next to yours. The count applies when the next match starts.");

        for (std::size_t count{ 1 }; count <= k_MaxLocalUsers; ++count)
        {
            ImGui::SameLine();

            const bool isCurrent{ count == previousCount };
            if (isCurrent) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            const bool isClicked{ ImGui::Button(std::to_string(count).c_str(), ImVec2{ k_CountButtonWidth, 0.0f }) };
            if (isCurrent) ImGui::PopStyleColor();

            if (!isClicked) continue;

            m_LocalUserStore.SetRequestedCount(count);

            // Without a real gamepad, Device pauses the game for an Artemis user (gamepad disconnected), so new users start Idle.
            for (std::size_t player{ previousCount }; player < count; ++player)
            {
                if (m_SimulationUIStore.GetLocalPlayerInput(player) != LocalPlayerInput::Device) continue;

                m_SimulationUIStore.SetLocalPlayerInput(player, LocalPlayerInput::Idle);
                this->ApplyLocalPlayerInput(player, LocalPlayerInput::Idle);
            }
        }

        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit };

        if (!ImGui::BeginTable("LocalPlayers", 4, flags)) return;

        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, k_NumberColumnWidth);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, k_NameColumnWidth);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, k_StateColumnWidth);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthFixed, k_InputComboWidth);
        ImGui::TableHeadersRow();

        const std::size_t requestedCount{ m_LocalUserStore.GetRequestedCount() };

        const auto tick{ m_TickStore.Acquire() };
        const Players* players{ tick && tick->Players ? tick->Players.get() : nullptr };

        for (std::size_t player{ 0 }; player < k_MaxLocalUsers; ++player)
        {
            this->DrawLocalPlayerRow(player, requestedCount, players);
        }

        ImGui::EndTable();
    }

    auto SimulationUIService::DrawLocalPlayerRow(std::size_t player, std::size_t requestedCount, const Players* players) -> void
    {
        ImGui::TableNextRow();

        // note: The local index comes from the player mapping of the engine (A9 M3), not from Artemis.
        const Player* engine{ FindLocalPlayer(players, player) };

        // The number cell spans the row, so the tooltip with the engine data covers all of it except the combo.
        ImGui::TableNextColumn();
        ImGui::Selectable(std::to_string(player + 1).c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

        if (engine)
        {
            const std::string tooltip{ engine->BipedHandle != k_NoHandle
                ? std::format("Engine player {:08X}, biped {:08X}", engine->Handle, engine->BipedHandle)
                : std::format("Engine player {:08X}, no biped", engine->Handle) };
            StatusGuiService::ShowTooltip(tooltip.c_str());
        }
        else
        {
            StatusGuiService::ShowTooltip("The engine has no player for this user.");
        }

        ImGui::TableNextColumn();
        if (player == 0)
        {
            ImGui::TextUnformatted("You (MCC)");
        }
        else
        {
            ImGui::TextUnformatted(ToNarrow(m_LocalUserStore.GetName(player)).c_str());
        }

        ImGui::TableNextColumn();
        if (player == 0)
        {
            ImGui::TextUnformatted("-");
        }
        else if (player >= requestedCount)
        {
            ImGui::TextDisabled("Off");
        }
        else
        {
            ImGui::TextUnformatted(m_LocalUserStore.IsReported(player) ? "In match" : "Waiting for a match");
        }

        ImGui::TableNextColumn();
        ImGui::PushID(static_cast<int>(player));

        const LocalPlayerInput current{ m_SimulationUIStore.GetLocalPlayerInput(player) };

        ImGui::SetNextItemWidth(k_InputComboWidth);
        if (ImGui::BeginCombo("##Input", k_LocalPlayerInputLabels[static_cast<std::size_t>(current)]))
        {
            for (std::size_t i{ 0 }; i < k_LocalPlayerInputCount; ++i)
            {
                const auto input{ static_cast<LocalPlayerInput>(i) };

                if (ImGui::Selectable(k_LocalPlayerInputLabels[i], input == current) && input != current)
                {
                    m_SimulationUIStore.SetLocalPlayerInput(player, input);
                    this->ApplyLocalPlayerInput(player, input);
                }
            }

            ImGui::EndCombo();
        }
        StatusGuiService::ShowTooltip("Input this user plays with. Device reads the real devices, the others are virtual.");

        ImGui::PopID();
    }

    auto SimulationUIService::ApplyLocalPlayerInput(std::size_t player, LocalPlayerInput input) -> void
    {
        if (input == LocalPlayerInput::Device)
        {
            m_VirtualInputStore.SetSource(player, LocalInputSource::Device);
            return;
        }

        m_VirtualInputStore.SetState(player, this->BuildTestState(input));
        m_VirtualInputStore.SetSource(player, LocalInputSource::Virtual);
    }

    auto SimulationUIService::DrawPlayerActions() -> void
    {
        // Command of a local player for the latest tick, as the simulation applies it. Press Clear, then a control in the game: yellow
        // bits and red bytes changed at some tick since the clear, so presses of a single tick are not missed (A9 M4, D109).
        int localPlayer{ static_cast<int>(m_SimulationUIStore.GetActionLocalPlayer()) + 1 };
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::SliderInt("Local player", &localPlayer, 1, static_cast<int>(k_MaxLocalUsers)))
        {
            m_SimulationUIStore.SetActionLocalPlayer(static_cast<std::size_t>(localPlayer - 1));
        }
        StatusGuiService::ShowTooltip("Local player whose action is shown.");

        const std::size_t local{ m_SimulationUIStore.GetActionLocalPlayer() };

        const auto tick{ m_TickStore.Acquire() };
        const Players* players{ tick && tick->Players ? tick->Players.get() : nullptr };

        const Player* engine{ FindLocalPlayer(players, local) };
        if (!engine)
        {
            ImGui::TextDisabled("The engine has no player for this local player.");
            return;
        }

        const std::size_t index{ engine->Handle & k_HandleIndexMask };
        if (index >= k_MaxActionPlayers) return;

        PlayerActions actions{};
        m_PlayerActionStore.CopyLatest(actions);

        if (actions.Sequence == 0)
        {
            ImGui::TextDisabled("No tick seen yet (the hook is installed when the engine starts).");
            return;
        }

        ActionWatch watch{};
        m_PlayerActionStore.CopyWatch(index, watch);

        const auto& action{ actions.Actions[index] };
        const bool hasAction{ (actions.Mask & (1u << index)) != 0 };
        const std::uint64_t buttons{ ReadAction<std::uint64_t>(action, k_ActionButtonsOffset) };

        ImGui::Text("Player %08X (table index %zu), tick %llu, action this tick: %s", engine->Handle, index,
            static_cast<unsigned long long>(actions.Sequence), hasAction ? "yes" : "no");

        const LocalPlayerAction current{ m_SimulationUIStore.GetLocalPlayerAction(local) };

        // note: The labels repeat other widgets of the tab (the Engine column), so the radios get their own ID scope.
        ImGui::PushID("ActionMode");
        if (const auto mode{ DrawActionRadios(current) })
        {
            m_SimulationUIStore.SetLocalPlayerAction(local, *mode);
            this->ApplyLocalPlayerAction(local, index, *mode, action);
        }
        ImGui::PopID();
        StatusGuiService::ShowTooltip("What the simulation writes for this player every tick instead of the action the engine built.");

        ImGui::Text("Buttons: %016llX", static_cast<unsigned long long>(buttons));
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) m_PlayerActionStore.ClearWatches();
        StatusGuiService::ShowTooltip("Clears the marks: yellow bits and red bytes changed at some tick since the clear.");
        ImGui::SameLine();
        if (ImGui::SmallButton("Log")) this->LogAction(local, actions.Sequence, action, watch);
        StatusGuiService::ShowTooltip("Writes the action and what changed since the last clear to the log.");

        this->DrawButtonBits(buttons, watch.SeenButtons);

        const float yaw{ ReadAction<float>(action, k_ActionFacingOffset) };
        const float pitch{ ReadAction<float>(action, k_ActionFacingOffset + sizeof(float)) };
        const float roll{ ReadAction<float>(action, k_ActionFacingOffset + (2 * sizeof(float))) };
        ImGui::Text("Facing: yaw %.4f (%.1f deg), pitch %.4f (%.1f deg), roll %.4f", yaw, yaw * k_RadiansToDegrees, pitch,
            pitch * k_RadiansToDegrees, roll);

        const float throttleForward{ ReadAction<float>(action, k_ActionThrottleOffset) };
        const float throttleLeft{ ReadAction<float>(action, k_ActionThrottleOffset + sizeof(float)) };
        ImGui::Text("Throttle: forward %.3f, left %.3f", throttleForward, throttleLeft);

        this->DrawActionBytes(action, watch);
    }

    auto SimulationUIService::DrawButtonBits(std::uint64_t buttons, std::uint64_t seen) -> void
    {
        for (std::size_t bit{ 0 }; bit < k_ButtonBits; ++bit)
        {
            if (bit % k_ButtonBitsPerRow != 0) ImGui::SameLine();

            const std::uint64_t mask{ std::uint64_t{ 1 } << bit };

            if ((buttons & mask) != 0)   ImGui::TextColored(k_HeldColor, "%02zu", bit);
            else if ((seen & mask) != 0) ImGui::TextColored(k_SeenColor, "%02zu", bit);
            else                         ImGui::TextDisabled("%02zu", bit);
        }
    }

    auto SimulationUIService::DrawActionBytes(const PlayerAction& action, const ActionWatch& watch) -> void
    {
        for (std::size_t offset{ 0 }; offset < k_PlayerActionSize; ++offset)
        {
            if (offset % k_BytesPerRow == 0)
            {
                ImGui::TextDisabled("+%02zX", offset);
            }

            ImGui::SameLine();

            const std::uint8_t value{ action.Data[offset] };

            if (watch.IsChanged[offset])
            {
                ImGui::TextColored(k_ChangedColor, "%02X", value);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("+0x%02zX: was %02X at the clear, last different value %02X.", offset,
                        watch.Reference.Data[offset], watch.LastChanged.Data[offset]);
                }
            }
            else
            {
                ImGui::Text("%02X", value);
            }
        }
    }

    auto BuildActionOverride(LocalPlayerAction mode, const PlayerAction& action, std::optional<std::size_t> localPlayer)
        -> std::optional<ActionOverride>
    {
        if (mode == LocalPlayerAction::Engine) return std::nullopt;

        ActionOverride actionOverride{};
        actionOverride.Yaw = ReadAction<float>(action, k_ActionFacingOffset);
        actionOverride.Pitch = ReadAction<float>(action, k_ActionFacingOffset + sizeof(float));
        actionOverride.LocalIndex = localPlayer;

        if (mode == LocalPlayerAction::TestAction)
        {
            actionOverride.ThrottleForward = k_TestThrottle;
            actionOverride.YawPerTick = k_TestYawPerTick;
        }

        actionOverride.IsUseTest = mode == LocalPlayerAction::TestUse;
        if (mode == LocalPlayerAction::HoldUse) actionOverride.Lanes = k_UseLane;

        return actionOverride;
    }

    auto SimulationUIService::ApplyLocalPlayerAction(std::size_t localPlayer, std::size_t playerIndex, LocalPlayerAction mode,
        const PlayerAction& action) -> void
    {
        m_PlayerActionStore.SetOverride(playerIndex, BuildActionOverride(mode, action, localPlayer));
    }

    auto SimulationUIService::LogAction(std::size_t localPlayer, std::uint64_t sequence, const PlayerAction& action,
        const ActionWatch& watch) -> void
    {
        std::string bytes{};
        bytes.reserve(k_PlayerActionSize * 2);

        for (const std::uint8_t value : action.Data)
        {
            bytes += std::format("{:02X}", value);
        }

        std::string changed{};

        for (std::size_t offset{ 0 }; offset < k_PlayerActionSize; ++offset)
        {
            if (!watch.IsChanged[offset]) continue;

            changed += std::format(" +{:02X}:{:02X}->{:02X}", offset, watch.Reference.Data[offset], watch.LastChanged.Data[offset]);
        }

        m_Log.Message("[SimulationUIService] INFO: Action of local player {} at tick {}: buttons seen {:016X},"
            " changed since the clear{}, bytes {}.", localPlayer + 1, sequence, watch.SeenButtons,
            changed.empty() ? std::string{ " none" } : changed, bytes);
    }

    auto SimulationUIService::DrawSession() -> void
    {
        // Machines and players as the engine has them. Up to 4 players per machine. Bots live on machines Artemis adds from slot 16 down.
        SessionSnapshot snapshot{};
        m_BotStore.CopySnapshot(snapshot);

        if (snapshot.Sequence == 0)
        {
            ImGui::TextDisabled("No tick seen yet.");
            return;
        }

        std::vector<FakeMachine> fakeMachines{};
        m_BotStore.CopyFakeMachines(fakeMachines);

        std::string machines{};

        for (std::size_t machine{ 0 }; machine < k_MaxMachines; ++machine)
        {
            if ((snapshot.MachineMask & (1u << machine)) == 0) continue;

            machines += machines.empty() ? std::format("{}", machine) : std::format(", {}", machine);
        }

        ImGui::Text("%d machines: %s", std::popcount(snapshot.MachineMask), machines.c_str());
        StatusGuiService::ShowTooltip(std::format("Tick {}, local machine {}. Machines Artemis added for its bots are listed as bot in the players.",
            snapshot.Sequence, snapshot.LocalMachineIndex).c_str());

        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit };
        const std::size_t picked{ m_SimulationUIStore.GetSessionPlayer() };

        if (ImGui::BeginTable("SessionPlayers", 4, flags))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, k_NumberColumnWidth);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, k_NameColumnWidth);
            ImGui::TableSetupColumn("Machine", ImGuiTableColumnFlags_WidthFixed, k_KindColumnWidth);
            ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, k_KindColumnWidth);
            ImGui::TableHeadersRow();

            for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
            {
                const SessionPlayer& player{ snapshot.Players[index] };
                if (!player.IsValid) continue;

                const bool isFake{ std::ranges::any_of(fakeMachines, [&player](const FakeMachine& fake) {
                    return std::cmp_equal(fake.Index, player.MachineIndex);
                }) };

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Selectable(std::to_string(index).c_str(), index == picked, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_SimulationUIStore.SetSessionPlayer(index);
                }

                const auto nameLength{ static_cast<std::size_t>(std::ranges::find(player.GamerTag, L'\0') - player.GamerTag.begin()) };

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ToNarrow(std::wstring_view{ player.GamerTag.data(), nameLength }).c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%d", player.MachineIndex);
                ImGui::TableNextColumn();
                if (player.MachineIndex == snapshot.LocalMachineIndex) ImGui::TextUnformatted("local");
                else if (isFake)                                         ImGui::TextUnformatted("bot");
                else                                                     ImGui::TextUnformatted("remote");
            }

            ImGui::EndTable();
        }

        if (picked < k_MaxPlayers && snapshot.Players[picked].IsValid)
        {
            const SessionPlayer& player{ snapshot.Players[picked] };

            ImGui::Text("Player %zu: handle %08X, identifier %016llX", picked, player.Handle,
                static_cast<unsigned long long>(player.Identifier));
            ImGui::Text("Flags %08X, user %d, controller %d", player.Flags, player.UserIndex, player.Controller);
        }
        else
        {
            ImGui::TextDisabled("Pick a player to see its detail.");
        }

        if (ImGui::Button("Log options")) this->LogOptions();
        StatusGuiService::ShowTooltip("Writes the game options entry of every valid player to the log, so the layout can be shared.");
    }

    auto SimulationUIService::LogOptions() -> void
    {
        SessionSnapshot snapshot{};
        m_BotStore.CopySnapshot(snapshot);

        for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            const auto& entry{ snapshot.Options[index] };
            if (entry[0] == 0) continue;

            m_Log.Message("[SimulationUIService] INFO: Game options of player {} at tick {}: {}.", index, snapshot.Sequence,
                ToHex(entry));
        }
    }

    auto SimulationUIService::DrawBots() -> void
    {
        // Bots are players on machines of their own, added in the match like a player that joins in progress (A9 M5b). They have no
        // view and no input: their action is the mode of the row. In Engine mode the engine repeats the last action it applied (as for a
        // remote player), so a bot keeps walking after Test action. The film does not have them.
        const std::optional<std::int8_t> newTeam{ m_SimulationUIStore.GetNewBotTeam() };

        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::BeginCombo("Team of new bots", newTeam ? GetTeamLabel(*newTeam) : "Same as the local player"))
        {
            if (ImGui::Selectable("Same as the local player", !newTeam)) m_SimulationUIStore.SetNewBotTeam(std::nullopt);

            for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
            {
                const auto value{ static_cast<std::int8_t>(team) };
                if (ImGui::Selectable(k_TeamLabels[team], newTeam == value)) m_SimulationUIStore.SetNewBotTeam(value);
            }

            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("Only team games use it. Other games give each player its own team.");

        ImGui::SameLine();
        if (ImGui::Button("Add bot")) m_BotStore.RequestBot(newTeam);
        ImGui::SetItemTooltip("Applied on the next tick of a match. The result is in the log.");

        this->DrawRoster();

        std::vector<Bot> bots{};
        m_BotStore.CopyBots(bots);

        if (bots.empty()) return;

        SessionSnapshot snapshot{};
        m_BotStore.CopySnapshot(snapshot);

        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg };

        PlayerActions actions{};
        m_PlayerActionStore.CopyLatest(actions);

        // note: Temporary until the bot editor (sub-commit 3) replaces this table: it scrolls instead of widening the window.
        if (!ImGui::BeginTable("Bots", 11, flags | ImGuiTableFlags_ScrollX)) return;

        ImGui::TableSetupColumn("Bot", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Player");
        ImGui::TableSetupColumn("Machine");
        ImGui::TableSetupColumn("User");
        ImGui::TableSetupColumn("In engine");
        ImGui::TableSetupColumn("Team");
        ImGui::TableSetupColumn("Unit (flags)");
        ImGui::TableSetupColumn("Action this tick");
        ImGui::TableSetupColumn("Action");
        ImGui::TableSetupColumn("Requests");
        ImGui::TableSetupColumn("Remove");
        ImGui::TableHeadersRow();

        for (const Bot& bot : bots)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%zu", bot.Number);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", bot.PlayerIndex);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", bot.MachineIndex);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", bot.Slot);

            const SessionPlayer& player{ snapshot.Players[bot.PlayerIndex] };

            ImGui::TableNextColumn();
            if (player.IsValid && player.Identifier == bot.Identifier) ImGui::Text("%08X", player.Handle);
            else                                                       ImGui::TextDisabled("Gone");

            ImGui::PushID(static_cast<int>(bot.PlayerIndex));

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##Team", GetTeamLabel(player.Team)))
            {
                for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
                {
                    const auto value{ static_cast<std::int8_t>(team) };
                    if (ImGui::Selectable(k_TeamLabels[team], player.Team == value) && player.Team != value)
                    {
                        m_BotStore.RequestTeam(TeamRequest{ .PlayerIndex = bot.PlayerIndex, .Team = value });
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::TableNextColumn();
            if (player.UnitHandle != k_NoHandle) ImGui::Text("%08X (%08X)", player.UnitHandle, player.UnitFlags);
            else                                 ImGui::TextDisabled("-");

            ImGui::TableNextColumn();
            const bool hasAction{ (actions.Mask & (1u << bot.PlayerIndex)) != 0 };
            ImGui::TextUnformatted(hasAction ? "yes" : "no");
            ImGui::SameLine();
            if (ImGui::SmallButton("Log")) this->LogBotAction(bot.Number, bot.PlayerIndex, actions);

            ImGui::TableNextColumn();

            const LocalPlayerAction current{ m_SimulationUIStore.GetBotAction(bot.PlayerIndex, player.Handle) };

            if (const auto mode{ DrawActionRadios(current) })
            {
                m_SimulationUIStore.SetBotAction(bot.PlayerIndex, player.Handle, *mode);
                this->ApplyBotAction(bot.PlayerIndex, *mode);
            }

            // note: One-tick presses of the client (D120). They only reach the bot while its action is not Engine.
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Weapon"))
            {
                m_PlayerActionStore.RequestClient(bot.PlayerIndex, ClientRequests{ .IsWeaponSwitch = true });
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Grenade"))
            {
                m_PlayerActionStore.RequestClient(bot.PlayerIndex, ClientRequests{ .IsGrenadeSwitch = true });
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Zoom")) m_PlayerActionStore.RequestClient(bot.PlayerIndex, ClientRequests{ .IsZoom = true });

            if (player.Client)
            {
                ImGui::Text("Slot %d, inventory %u, grenade %d, zoom %d%s", player.Client->WeaponSlot,
                    player.Client->InventoryVersion, player.Client->GrenadeType, player.Client->ZoomLevel,
                    player.Client->IsSeated ? ", seated" : "");
            }

            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Remove")) m_BotStore.RequestRemove(bot.Number);
            ImGui::SetItemTooltip("The bot leaves the match on the next tick and its slot is free at once. The result is in the log.");

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    auto SimulationUIService::DrawShadow() -> void
    {
        // For every local player without an override, compares what Artemis would send as its client with what the engine built (D122).
        // Play normally: switch weapons and grenades, zoom, hold buttons, walk over objects.
        bool isEnabled{ m_PlayerActionStore.IsShadowEnabled() };
        if (ImGui::Checkbox("Enabled", &isEnabled)) m_PlayerActionStore.SetShadowEnabled(isEnabled);
        StatusGuiService::ShowTooltip(std::format("Compares what Artemis would send as the client of each local player without an override"
            " with what the engine built. Mismatches logged per field: {}.", k_ShadowLogLimit).c_str());

        ImGui::SameLine();
        if (ImGui::Button("Clear")) m_PlayerActionStore.ClearShadow();
        StatusGuiService::ShowTooltip("Sets every counter to zero.");

        std::array<ShadowCount, k_ShadowFieldCount> counts{};
        m_PlayerActionStore.CopyShadow(counts);

        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg };
        if (!ImGui::BeginTable("Shadow", 3, flags)) return;

        ImGui::TableSetupColumn("Field");
        ImGui::TableSetupColumn("Checked");
        ImGui::TableSetupColumn("Mismatched");
        ImGui::TableHeadersRow();

        for (std::size_t field{ 0 }; field < k_ShadowFieldCount; ++field)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(k_ShadowFieldNames[field]);
            ImGui::TableNextColumn();
            ImGui::Text("%llu", static_cast<unsigned long long>(counts[field].Checked));
            ImGui::TableNextColumn();
            ImGui::Text("%llu", static_cast<unsigned long long>(counts[field].Mismatched));
        }

        ImGui::EndTable();
    }

    auto SimulationUIService::LogBotAction(std::size_t number, std::size_t playerIndex, const PlayerActions& actions) -> void
    {
        m_Log.Message("[SimulationUIService] INFO: Action of bot {} (player {}) at tick {}: in the mask {}, bytes {}.", number,
            playerIndex, actions.Sequence, (actions.Mask & (1u << playerIndex)) != 0 ? "yes" : "no",
            ToHex(actions.Actions[playerIndex].Data));
    }

    auto SimulationUIService::DrawRoster() -> void
    {
        const std::string line{ ToText(m_MatchSetupService.GetActive(), m_MatchSetupService.GetResult()) };
        ImGui::TextWrapped("%s", line.c_str());

        Roster editor{ m_SimulationUIStore.GetRosterEditor() };
        bool isEdited{ false };

        for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
        {
            int count{ static_cast<int>(editor.Teams[team]) };
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::InputInt(k_TeamLabels[team], &count, 1, 4))
            {
                editor.Teams[team] = static_cast<std::size_t>((std::clamp)(count, 0, static_cast<int>(k_MaxPlayers)));
                isEdited = true;
            }
        }

        int autoCount{ static_cast<int>(editor.Auto) };
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::InputInt("Auto", &autoCount, 1, 4))
        {
            editor.Auto = static_cast<std::size_t>((std::clamp)(autoCount, 0, static_cast<int>(k_MaxPlayers)));
            isEdited = true;
        }

        if (isEdited) m_SimulationUIStore.SetRosterEditor(editor);

        if (ImGui::Button("Request roster")) m_MatchSetupService.Request(MatchSetup{ .Bots = editor }, MatchSetupOrigin::Ui);
    }

    auto SimulationUIService::ApplyBotAction(std::size_t playerIndex, LocalPlayerAction mode) -> void
    {
        PlayerActions actions{};
        m_PlayerActionStore.CopyLatest(actions);

        m_PlayerActionStore.SetOverride(playerIndex, BuildActionOverride(mode, actions.Actions[playerIndex], std::nullopt));
    }

    auto SimulationUIService::BuildTestState(LocalPlayerInput input) -> InputState
    {
        InputState state{};

        switch (input)
        {
        case LocalPlayerInput::TestKeyboardMouse:
            state.IsKeyboardMouse = 1;
            state.Keys[k_ForwardKey] = true;
            state.Mouse.DeltaX = k_TestMouseDelta;
            break;
        case LocalPlayerInput::TestGamepad:
            state.Gamepad.LeftY = k_StickForward;
            state.Gamepad.RightX = k_TestStickTurn;
            break;
        default:
            break;
        }

        return state;
    }
}