module;

#include "External/imgui/imgui.h"

module DevTools.Workbench.Simulation.System;

import Common.Engine.Type;
import Common.Team.Type;
import Control.Action.Type;
import Engine.Base.Lifecycle.Type;
import Engine.Base.MatchSetup.Type;
import Engine.Control.Simulation.Type;
import Bridge.Transport.Type;
import Blam.Layout.Action.Type;
import Perception.Match.Type;
import DevTools.Gui.Color.System;
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
    using Engine::Base::MatchSetup::Type::MatchSetupOutcome;
    using Engine::Base::MatchSetup::Type::MatchSetupResult;
    using Engine::Base::MatchSetup::Type::Roster;
    using Engine::Base::MatchSetup::Type::ToText;
    using DevTools::Workbench::Simulation::Type::LocalPlayerAction;
    using DevTools::Workbench::Simulation::Type::SimulationSection;
    using Common::Team::Type::Team;
    using DevTools::Gui::Color::System::TeamToColor;
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

    // Widths of the columns of the bot table and characters of the chip of the last request.
    constexpr float k_BotModeColumnWidth{ 110.0f };
    constexpr std::size_t k_MaxChipCharacters{ 56 };

    // return: The color of a team index of the engine, grey outside 0..7.
    auto GetTeamColor(std::int8_t team) -> ImVec4
    {
        const bool isTeam{ team >= 0 && static_cast<std::size_t>(team) < k_MaxTeams };
        return TeamToColor::TeamColorVec4(isTeam ? static_cast<Team>(team) : Team::Neutral, 1.0f);
    }

    // return: Width of a counter drawn by DrawCounter, to place it in a line.
    auto CounterWidth(const char* label) -> float
    {
        const ImGuiStyle& style{ ImGui::GetStyle() };
        const float button{ ImGui::CalcTextSize("-").x + (style.FramePadding.x * 2.0f) };

        return ImGui::CalcTextSize(label).x + ImGui::CalcTextSize("00").x + (button * 2.0f) + (style.ItemSpacing.x * 3.0f);
    }

    // Draws a label with a number between a minus and a plus button.
    // return: Whether the number changed.
    auto DrawCounter(const char* label, std::size_t& value, const char* tooltip) -> bool
    {
        bool isChanged{ false };

        ImGui::PushID(label);
        ImGui::BeginGroup();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine();

        if (ImGui::SmallButton("-") && value > 0)
        {
            --value;
            isChanged = true;
        }

        ImGui::SameLine();
        ImGui::Text("%2zu", value);
        ImGui::SameLine();

        if (ImGui::SmallButton("+") && value < k_MaxPlayers)
        {
            ++value;
            isChanged = true;
        }

        ImGui::EndGroup();
        ImGui::PopID();

        StatusGuiService::ShowTooltip(tooltip);
        return isChanged;
    }

    // return: The text cut to k_MaxChipCharacters with "..." when longer, so a chip fits the width of the panel.
    auto Shorten(std::string text) -> std::string
    {
        if (text.size() <= k_MaxChipCharacters) return text;

        text.resize(k_MaxChipCharacters - 3);
        return text + "...";
    }

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
        // view and no input: their action is the mode of the picked bot. In Engine mode the engine repeats the last action it applied
        // (as for a remote player), so a bot keeps walking after Test action. The film does not have them.
        // Not done on purpose: the read-only mode when the orchestrator is connected (U14), which edits the roster from one instance only.
        // The orchestrator does not exist yet, so nothing here checks for it.
        this->DrawRoster();

        std::vector<Bot> bots{};
        m_BotStore.CopyBots(bots);

        if (bots.empty())
        {
            ImGui::TextDisabled("No bots in the match.");
            return;
        }

        SessionSnapshot snapshot{};
        m_BotStore.CopySnapshot(snapshot);

        this->DrawBotTable(bots, snapshot);

        const auto picked{ std::ranges::find(bots, m_SimulationUIStore.GetSelectedBot(), &Bot::Number) };

        if (picked == bots.end())
        {
            ImGui::TextDisabled("Pick a bot to see its detail.");
            return;
        }

        this->DrawBotDetail(*picked, snapshot.Players[picked->PlayerIndex]);
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
        // The counters are the bots each team should have in total, not bots to add. Apply now and Next round both go through the match
        // setup as a request with only the roster, so an applied roster is the active one and the next match applies it again.
        Roster editor{ m_SimulationUIStore.GetRosterEditor() };
        bool isEdited{ false };

        ControlFlow flow{};

        for (const std::size_t team : { std::size_t{ 0 }, std::size_t{ 1 } })
        {
            flow.Place(CounterWidth(k_TeamLabels[team]));
            isEdited |= DrawCounter(k_TeamLabels[team], editor.Teams[team], "Bots on this team, in total.");
        }

        flow.Place(CounterWidth("Auto"));
        isEdited |= DrawCounter("Auto", editor.Auto, "Bots that go to the team of the first local player, in total.");

        if (ImGui::TreeNode("Other teams"))
        {
            ControlFlow others{};

            for (std::size_t team{ 2 }; team < k_MaxTeams; ++team)
            {
                others.Place(CounterWidth(k_TeamLabels[team]));
                isEdited |= DrawCounter(k_TeamLabels[team], editor.Teams[team], "Bots on this team, in total.");
            }

            ImGui::TreePop();
        }

        if (isEdited) m_SimulationUIStore.SetRosterEditor(editor);

        ControlFlow buttons{};

        // A request waiting from the bridge or the bench would be replaced by ours without a word: the buttons wait for it instead.
        const std::optional<MatchSetupOrigin> waiting{ m_MatchSetupService.GetPendingOrigin() };
        const bool isOtherWaiting{ waiting.has_value() && *waiting != MatchSetupOrigin::Ui };
        const std::string otherReason{ isOtherWaiting ? std::format("A setup from {} is waiting", ToText(*waiting)) : std::string{} };

        {
            const bool isInMatch{ m_LifecycleStore.GetStatus() == Engine::Base::Lifecycle::Type::Status::Running };
            const DisabledScope noMatch{ isOtherWaiting || !isInMatch, isOtherWaiting ? otherReason.c_str() : "Needs a match" };

            if (buttons.Button("Apply now")) m_MatchSetupService.Request(MatchSetup{ .Bots = editor, .IsImmediate = true }, MatchSetupOrigin::Ui);
            noMatch.Tooltip("Turns the bots of the match into this roster on the next frame, in the middle of the round if need be.");
        }

        {
            const DisabledScope otherWaiting{ isOtherWaiting, otherReason.c_str() };

            if (buttons.Button("Next round")) m_MatchSetupService.Request(MatchSetup{ .Bots = editor }, MatchSetupOrigin::Ui);
            otherWaiting.Tooltip("Applies this roster at the end of the round, or at the next load if no match is running.");
        }

        // The last request: waiting, applied (the round it went in) or rejected with its reason. The tooltip has the whole setup line.
        const MatchSetupResult result{ m_MatchSetupService.GetResult() };
        const std::string line{ ToText(m_MatchSetupService.GetActive(), result) };

        switch (result.Outcome)
        {
        case MatchSetupOutcome::None:
            StatusGuiService::DrawChip("No request yet", StatusKind::Off, line.c_str());
            break;
        case MatchSetupOutcome::Pending:
            StatusGuiService::DrawChip("Pending", StatusKind::Pending, line.c_str());
            break;
        case MatchSetupOutcome::AtLoad:
        case MatchSetupOutcome::Applied:
        {
            const std::string text{ result.Reason.empty() ? std::format("Applied (round {})", result.Round)
                : std::format("Applied (round {}): {}", result.Round, result.Reason) };
            StatusGuiService::DrawChip(Shorten(text).c_str(), result.Reason.empty() ? StatusKind::Ok : StatusKind::Pending, line.c_str());
            break;
        }
        case MatchSetupOutcome::Rejected:
            StatusGuiService::DrawChip(Shorten("Rejected: " + result.Reason).c_str(), StatusKind::Error, line.c_str());
            break;
        }
    }

    auto SimulationUIService::DrawBotTable(const std::vector<Bot>& bots, const SessionSnapshot& snapshot) -> void
    {
        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit };

        if (!ImGui::BeginTable("Bots", 4, flags)) return;

        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, k_NumberColumnWidth);
        ImGui::TableSetupColumn("Team", ImGuiTableColumnFlags_WidthFixed, k_NameColumnWidth);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, k_StateColumnWidth);
        ImGui::TableSetupColumn("Mode", ImGuiTableColumnFlags_WidthFixed, k_BotModeColumnWidth);
        ImGui::TableHeadersRow();

        const auto tick{ m_TickStore.Acquire() };
        const Players* players{ tick && tick->Players ? tick->Players.get() : nullptr };
        const std::size_t picked{ m_SimulationUIStore.GetSelectedBot() };

        for (const Bot& bot : bots)
        {
            const SessionPlayer& player{ snapshot.Players[bot.PlayerIndex] };
            const bool isInEngine{ player.IsValid && player.Identifier == bot.Identifier };

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Selectable(std::to_string(bot.Number).c_str(), bot.Number == picked, ImGuiSelectableFlags_SpanAllColumns))
            {
                m_SimulationUIStore.SetSelectedBot(bot.Number);
            }

            ImGui::TableNextColumn();
            ImGui::TextColored(GetTeamColor(player.Team), "%s", GetTeamLabel(player.Team));

            // The state: gone (no longer in the engine), then alive or dead by the live body of the tick. The tick does not say who
            // carries an objective without more work, so there is no "flag" state yet.
            ImGui::TableNextColumn();
            if (!isInEngine)
            {
                ImGui::TextDisabled("gone");
            }
            else
            {
                const Player* seen{ players ? players->Find(player.Handle) : nullptr };
                ImGui::TextUnformatted(seen != nullptr && seen->IsAlive() ? "alive" : "dead");
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(k_LocalPlayerActionLabels[static_cast<std::size_t>(
                m_SimulationUIStore.GetBotAction(bot.PlayerIndex, player.Handle))]);
        }

        ImGui::EndTable();
    }

    auto SimulationUIService::DrawBotDetail(const Bot& bot, const SessionPlayer& player) -> void
    {
        const bool isInEngine{ player.IsValid && player.Identifier == bot.Identifier };

        PlayerActions actions{};
        m_PlayerActionStore.CopyLatest(actions);

        ImGui::PushID(static_cast<int>(bot.PlayerIndex));
        ImGui::SeparatorText(std::format("Bot {}", bot.Number).c_str());

        const std::string handle{ isInEngine ? std::format("{:08X}", player.Handle) : std::string{ "gone" } };
        ImGui::Text("Player %zu (%s), machine %zu, user %zu", bot.PlayerIndex, handle.c_str(), bot.MachineIndex, bot.Slot);

        if (player.UnitHandle != k_NoHandle) ImGui::Text("Unit %08X (flags %08X)", player.UnitHandle, player.UnitFlags);
        else                                 ImGui::TextDisabled("No unit");

        const bool hasAction{ (actions.Mask & (1u << bot.PlayerIndex)) != 0 };
        ImGui::Text("Action this tick: %s", hasAction ? "yes" : "no");

        if (player.Client)
        {
            ImGui::Text("Slot %d, inventory %u, grenade %d, zoom %d%s", player.Client->WeaponSlot, player.Client->InventoryVersion,
                player.Client->GrenadeType, player.Client->ZoomLevel, player.Client->IsSeated ? ", seated" : "");
        }

        // Changing the team asks the engine on the next tick, the same request the roster uses for a move.
        ImGui::SetNextItemWidth(k_InputComboWidth);
        if (ImGui::BeginCombo("Team", GetTeamLabel(player.Team)))
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
        StatusGuiService::ShowTooltip("Moves this bot to another team on the next tick.");

        const LocalPlayerAction current{ m_SimulationUIStore.GetBotAction(bot.PlayerIndex, player.Handle) };

        ImGui::PushID("BotMode");
        if (const auto mode{ DrawActionRadios(current) })
        {
            m_SimulationUIStore.SetBotAction(bot.PlayerIndex, player.Handle, *mode);
            this->ApplyBotAction(bot.PlayerIndex, *mode);
        }
        ImGui::PopID();
        StatusGuiService::ShowTooltip("What the bot does each tick. Engine repeats the last action the engine applied.");

        // note: One-tick presses of the client. They only reach the bot while its action is not Engine.
        ControlFlow flow{};

        if (flow.Button("Weapon")) m_PlayerActionStore.RequestClient(bot.PlayerIndex, ClientRequests{ .IsWeaponSwitch = true });
        StatusGuiService::ShowTooltip("Presses the weapon switch for one tick. Needs a mode other than Engine.");

        if (flow.Button("Grenade")) m_PlayerActionStore.RequestClient(bot.PlayerIndex, ClientRequests{ .IsGrenadeSwitch = true });
        StatusGuiService::ShowTooltip("Presses the grenade switch for one tick. Needs a mode other than Engine.");

        if (flow.Button("Zoom")) m_PlayerActionStore.RequestClient(bot.PlayerIndex, ClientRequests{ .IsZoom = true });
        StatusGuiService::ShowTooltip("Presses zoom for one tick. Needs a mode other than Engine.");

        if (flow.Button("Log")) this->LogBotAction(bot.Number, bot.PlayerIndex, actions);
        StatusGuiService::ShowTooltip("Writes the latest action of this bot to the log.");

        // Remove goes to the right end of the line, in red.
        const float removeWidth{ ImGui::CalcTextSize("Remove").x + (ImGui::GetStyle().FramePadding.x * 2.0f) };
        flow.Place(removeWidth);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (std::max)(0.0f, ImGui::GetContentRegionAvail().x - removeWidth));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.65f, 0.15f, 0.15f, 1.0f });
        if (ImGui::Button("Remove")) m_BotStore.RequestRemove(bot.Number);
        ImGui::PopStyleColor();
        StatusGuiService::ShowTooltip("The bot leaves the match on the next tick and its slot is free at once. The result is in the log.");

        ImGui::PopID();
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