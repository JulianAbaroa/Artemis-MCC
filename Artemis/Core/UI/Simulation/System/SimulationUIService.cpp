module;

#include "External/imgui/imgui.h"

module UI.Simulation.System;

import Platform.Host.Type;
import Platform.Simulation.Type;
import std;

namespace
{
    using Platform::Input::Type::LocalInputSource;
    using Platform::Input::Type::PlayerAction;
    using Platform::Input::Type::PlayerActions;
    using Platform::Input::Type::ActionOverride;
    using Export::Tick::Type::Player;
    using Export::Tick::Type::Players;
    using Platform::Simulation::Type::SessionSnapshot;
    using Platform::Simulation::Type::SessionPlayer;
    using Platform::Simulation::Type::Bot;
    using Platform::Simulation::Type::FakeMachine;
    using Platform::Simulation::Type::TeamRequest;

    using Platform::Host::Type::k_MaxLocalUsers;
    using Platform::Simulation::Type::k_MaxMachines;
    using Platform::Simulation::Type::k_MaxPlayers;
    using Platform::Simulation::Type::k_MaxTeams;
    using Platform::Input::Type::k_MaxActionPlayers;
    using Platform::Input::Type::k_PlayerActionSize;
    using Platform::Input::Type::k_ActionButtonsOffset;
    using Platform::Input::Type::k_ActionFacingOffset;
    using Platform::Input::Type::k_ActionThrottleOffset;
    using UI::Simulation::Type::k_LocalPlayerInputCount;
    using UI::Simulation::Type::k_LocalPlayerInputLabels;
    using UI::Simulation::Type::k_LocalPlayerActionCount;
    using UI::Simulation::Type::k_LocalPlayerActionLabels;

    // Logic ticks per second at x1.
    constexpr float k_TicksPerSecond{ 60.0f };

    // Speeds tested in A8 (0.1 to 100). Above x5 the engine caps at 10 ticks per frame.
    constexpr float k_MinTimeScale{ 0.1f };
    constexpr float k_MaxTimeScale{ 100.0f };
    constexpr std::array<float, 7> k_TimeScalePresets{ 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f };

    // Virtual-key code of W. The engine button W (32) reads it through its key table.
    constexpr std::size_t k_ForwardKey{ 0x57 };

    // Mouse movement delivered on every frame by the keyboard and mouse test, in mouse counts.
    constexpr float k_TestMouseDelta{ 1.0f };

    // Stick values of the gamepad test: full forward on the left stick and a steady turn on the right stick.
    constexpr std::int16_t k_StickForward{ 32767 };
    constexpr std::int16_t k_TestStickTurn{ 12000 };

    // Handle of no biped.
    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };

    // Mask of the table index inside a player handle.
    constexpr std::uint32_t k_IndexMask{ 0xFFFF };

    // Bits of the buttons of an action, and bits drawn per row.
    constexpr std::size_t k_ButtonBits{ 64 };
    constexpr std::size_t k_ButtonBitsPerRow{ 16 };

    // Bytes of the action drawn per row.
    constexpr std::size_t k_BytesPerRow{ 16 };

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

namespace UI::Simulation::System
{
    auto SimulationUIService::Draw() -> void
    {
        ImGui::SeparatorText("Game speed");

        this->DrawGameSpeed();

        ImGui::Spacing();
        ImGui::SeparatorText("Local players");

        this->DrawLocalPlayers();

        ImGui::Spacing();
        ImGui::SeparatorText("Player actions");

        this->DrawPlayerActions();

        ImGui::Spacing();
        ImGui::SeparatorText("Session");

        this->DrawSession();

        ImGui::Spacing();
        ImGui::SeparatorText("Bots");

        this->DrawBots();
    }

    auto SimulationUIService::DrawGameSpeed() -> void
    {
        ImGui::TextWrapped("Engine time scale. Above x5 the engine runs at most 10 ticks per frame, so the real speed is capped at"
            " fps x 10 / 60. Not saved: Artemis always starts without changing the speed.");

        const float requested{ m_LifecycleStore.GetRequestedTimeScale() };

        if (requested > 0.0f)
        {
            ImGui::Text("Requested: x%.2f", requested);
        }
        else
        {
            ImGui::TextUnformatted("Requested: engine default");
        }

        const float tickHz{ m_TelemetryStore.m_TickHz.load(std::memory_order_relaxed) };
        ImGui::Text("Real: %.0f ticks/s (x%.2f)", tickHz, tickHz / k_TicksPerSecond);

        if (ImGui::Button("Engine")) m_LifecycleStore.SetRequestedTimeScale(0.0f);
        ImGui::SetItemTooltip("Stop writing the speed. The engine goes back to x1.");

        for (const float preset : k_TimeScalePresets)
        {
            ImGui::SameLine();

            const std::string label{ std::format("x{:g}", preset) };
            if (ImGui::Button(label.c_str())) m_LifecycleStore.SetRequestedTimeScale(preset);
        }

        float custom{ m_SimulationUIStore.GetCustomTimeScale() };

        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputFloat("##CustomTimeScale", &custom, 0.0f, 0.0f, "%.2f"))
        {
            m_SimulationUIStore.SetCustomTimeScale(std::clamp(custom, k_MinTimeScale, k_MaxTimeScale));
        }

        ImGui::SameLine();
        if (ImGui::Button("Apply")) m_LifecycleStore.SetRequestedTimeScale(m_SimulationUIStore.GetCustomTimeScale());
    }

    auto SimulationUIService::DrawLocalPlayers() -> void
    {
        ImGui::TextWrapped("Split-screen users Artemis adds next to yours. The count applies when the next match starts.");

        const std::size_t previousCount{ m_LocalUserStore.GetRequestedCount() };
        int count{ static_cast<int>(previousCount) };

        if (ImGui::SliderInt("Players", &count, 1, static_cast<int>(k_MaxLocalUsers)))
        {
            m_LocalUserStore.SetRequestedCount(static_cast<std::size_t>(count));

            // Without a real gamepad, Device pauses the game for an Artemis user (gamepad disconnected), so new users start Idle.
            for (std::size_t player{ previousCount }; player < static_cast<std::size_t>(count); ++player)
            {
                if (m_SimulationUIStore.GetLocalPlayerInput(player) != LocalPlayerInput::Device) continue;

                m_SimulationUIStore.SetLocalPlayerInput(player, LocalPlayerInput::Idle);
                this->ApplyLocalPlayerInput(player, LocalPlayerInput::Idle);
            }
        }

        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg };

        if (!ImGui::BeginTable("LocalPlayers", 6, flags)) return;

        ImGui::TableSetupColumn("Player", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Engine");
        ImGui::TableSetupColumn("Handle");
        ImGui::TableSetupColumn("Biped");
        ImGui::TableSetupColumn("Input");
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

        ImGui::TableNextColumn();
        ImGui::Text("%zu", player + 1);

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

        // note: The local index comes from the player mapping of the engine (A9 M3), not from Artemis.
        const Player* engine{ FindLocalPlayer(players, player) };

        ImGui::TableNextColumn();
        if (engine) ImGui::Text("%08X", engine->Handle);
        else        ImGui::TextDisabled("-");

        ImGui::TableNextColumn();
        if (engine && engine->BipedHandle != k_NoHandle) ImGui::Text("%08X", engine->BipedHandle);
        else                                              ImGui::TextDisabled("-");

        ImGui::TableNextColumn();
        ImGui::PushID(static_cast<int>(player));

        const LocalPlayerInput current{ m_SimulationUIStore.GetLocalPlayerInput(player) };

        ImGui::SetNextItemWidth(-1.0f);
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
        ImGui::TextWrapped("Command of a local player for the latest tick, as the simulation applies it (A9 M4, D109). Press Clear,"
            " then a control in the game: yellow bits and red bytes changed at some tick since the clear, so presses of a single"
            " tick are not missed. The mode writes the action of the player every tick instead of the engine.");

        int localPlayer{ static_cast<int>(m_SimulationUIStore.GetActionLocalPlayer()) + 1 };
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::SliderInt("Local player", &localPlayer, 1, static_cast<int>(k_MaxLocalUsers)))
        {
            m_SimulationUIStore.SetActionLocalPlayer(static_cast<std::size_t>(localPlayer - 1));
        }

        const std::size_t local{ m_SimulationUIStore.GetActionLocalPlayer() };

        const auto tick{ m_TickStore.Acquire() };
        const Players* players{ tick && tick->Players ? tick->Players.get() : nullptr };

        const Player* engine{ FindLocalPlayer(players, local) };
        if (!engine)
        {
            ImGui::TextDisabled("The engine has no player for this local player.");
            return;
        }

        const std::size_t index{ engine->Handle & k_IndexMask };
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
        for (std::size_t i{ 0 }; i < k_LocalPlayerActionCount; ++i)
        {
            if (i != 0) ImGui::SameLine();

            const auto mode{ static_cast<LocalPlayerAction>(i) };
            if (ImGui::RadioButton(k_LocalPlayerActionLabels[i], mode == current) && mode != current)
            {
                m_SimulationUIStore.SetLocalPlayerAction(local, mode);
                this->ApplyLocalPlayerAction(local, index, mode, action);
            }
        }
        ImGui::PopID();

        ImGui::Text("Buttons: %016llX", static_cast<unsigned long long>(buttons));
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) m_PlayerActionStore.ClearWatches();
        ImGui::SameLine();
        if (ImGui::SmallButton("Log")) this->LogAction(local, actions.Sequence, action, watch);

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

    auto SimulationUIService::ApplyLocalPlayerAction(std::size_t localPlayer, std::size_t playerIndex, LocalPlayerAction mode,
        const PlayerAction& action) -> void
    {
        if (mode == LocalPlayerAction::Engine)
        {
            m_PlayerActionStore.SetOverride(playerIndex, std::nullopt);
            return;
        }

        ActionOverride actionOverride{};
        actionOverride.Yaw = ReadAction<float>(action, k_ActionFacingOffset);
        actionOverride.Pitch = ReadAction<float>(action, k_ActionFacingOffset + sizeof(float));
        actionOverride.LocalIndex = localPlayer;

        if (mode == LocalPlayerAction::TestAction)
        {
            actionOverride.ThrottleForward = k_TestThrottle;
            actionOverride.YawPerTick = k_TestYawPerTick;
        }

        m_PlayerActionStore.SetOverride(playerIndex, actionOverride);
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

        m_LogsService.Message("[SimulationUIService] INFO: Action of local player {} at tick {}: buttons seen {:016X},"
            " changed since the clear{}, bytes {}.", localPlayer + 1, sequence, watch.SeenButtons,
            changed.empty() ? std::string{ " none" } : changed, bytes);
    }

    auto SimulationUIService::DrawSession() -> void
    {
        ImGui::TextWrapped("Machines and players as the engine has them (A9 M5a). Up to 4 players per machine. Bots live on machines"
            " Artemis adds from slot 16 down.");

        SessionSnapshot snapshot{};
        m_BotStore.CopySnapshot(snapshot);

        if (snapshot.Sequence == 0)
        {
            ImGui::TextDisabled("No tick seen yet.");
            return;
        }

        std::vector<FakeMachine> fakeMachines{};
        m_BotStore.CopyFakeMachines(fakeMachines);

        ImGui::Text("Tick %llu, machine mask %05X, local machine %d", static_cast<unsigned long long>(snapshot.Sequence),
            snapshot.MachineMask, snapshot.LocalMachineIndex);

        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg };

        if (ImGui::BeginTable("Machines", 3, flags))
        {
            ImGui::TableSetupColumn("Machine", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Identifier");
            ImGui::TableSetupColumn("Kind");
            ImGui::TableHeadersRow();

            for (std::size_t machine{ 0 }; machine < k_MaxMachines; ++machine)
            {
                if ((snapshot.MachineMask & (1u << machine)) == 0) continue;

                const bool isFake{ std::ranges::any_of(fakeMachines, [machine](const FakeMachine& fake) {
                    return fake.Index == machine;
                }) };

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%zu", machine);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ToHex(snapshot.Machines[machine].Bytes).c_str());

                ImGui::TableNextColumn();
                if (static_cast<std::int32_t>(machine) == snapshot.LocalMachineIndex) ImGui::TextUnformatted("Local");
                else if (isFake)                                                       ImGui::TextUnformatted("Artemis (bots)");
                else                                                                   ImGui::TextUnformatted("Remote");
            }

            ImGui::EndTable();
        }

        if (ImGui::BeginTable("SessionPlayers", 8, flags))
        {
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Handle");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Flags");
            ImGui::TableSetupColumn("Identifier");
            ImGui::TableSetupColumn("Machine");
            ImGui::TableSetupColumn("User");
            ImGui::TableSetupColumn("Controller");
            ImGui::TableHeadersRow();

            for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
            {
                const SessionPlayer& player{ snapshot.Players[index] };
                if (!player.IsValid) continue;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%zu", index);
                ImGui::TableNextColumn();
                ImGui::Text("%08X", player.Handle);
                ImGui::TableNextColumn();
                const auto nameLength{ static_cast<std::size_t>(std::ranges::find(player.GamerTag, L'\0') - player.GamerTag.begin()) };
                ImGui::TextUnformatted(ToNarrow(std::wstring_view{ player.GamerTag.data(), nameLength }).c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%08X", player.Flags);
                ImGui::TableNextColumn();
                ImGui::Text("%016llX", static_cast<unsigned long long>(player.Identifier));
                ImGui::TableNextColumn();
                ImGui::Text("%d", player.MachineIndex);
                ImGui::TableNextColumn();
                ImGui::Text("%d", player.UserIndex);
                ImGui::TableNextColumn();
                ImGui::Text("%d", player.Controller);
            }

            ImGui::EndTable();
        }

        int optionsPlayer{ static_cast<int>(m_SimulationUIStore.GetOptionsPlayer()) };
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::SliderInt("Game options of player", &optionsPlayer, 0, static_cast<int>(k_MaxPlayers) - 1))
        {
            m_SimulationUIStore.SetOptionsPlayer(static_cast<std::size_t>(optionsPlayer));
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Log options")) this->LogOptions();

        const auto& entry{ snapshot.Options[m_SimulationUIStore.GetOptionsPlayer()] };

        for (std::size_t offset{ 0 }; offset < entry.size(); ++offset)
        {
            if (offset % k_BytesPerRow == 0) ImGui::TextDisabled("+%02zX", offset);

            ImGui::SameLine();
            ImGui::Text("%02X", entry[offset]);
        }
    }

    auto SimulationUIService::LogOptions() -> void
    {
        SessionSnapshot snapshot{};
        m_BotStore.CopySnapshot(snapshot);

        for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
        {
            const auto& entry{ snapshot.Options[index] };
            if (entry[0] == 0) continue;

            m_LogsService.Message("[SimulationUIService] INFO: Game options of player {} at tick {}: {}.", index, snapshot.Sequence,
                ToHex(entry));
        }
    }

    auto SimulationUIService::DrawBots() -> void
    {
        ImGui::TextWrapped("Players on machines of their own, added in the match like a player that joins in progress (A9 M5b)."
            " They have no view and no input: their action is the mode below. In Engine mode the engine repeats the last action it"
            " applied (as for a remote player), so a bot keeps walking after Test action. The film does not have them.");

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

        std::vector<Bot> bots{};
        m_BotStore.CopyBots(bots);

        if (bots.empty()) return;

        SessionSnapshot snapshot{};
        m_BotStore.CopySnapshot(snapshot);

        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg };

        PlayerActions actions{};
        m_PlayerActionStore.CopyLatest(actions);

        if (!ImGui::BeginTable("Bots", 9, flags)) return;

        ImGui::TableSetupColumn("Bot", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Player");
        ImGui::TableSetupColumn("Machine");
        ImGui::TableSetupColumn("User");
        ImGui::TableSetupColumn("In engine");
        ImGui::TableSetupColumn("Team");
        ImGui::TableSetupColumn("Unit (flags)");
        ImGui::TableSetupColumn("Action this tick");
        ImGui::TableSetupColumn("Action");
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

            const LocalPlayerAction current{ m_SimulationUIStore.GetBotAction(bot.PlayerIndex) };

            for (std::size_t i{ 0 }; i < k_LocalPlayerActionCount; ++i)
            {
                if (i != 0) ImGui::SameLine();

                const auto mode{ static_cast<LocalPlayerAction>(i) };
                if (ImGui::RadioButton(k_LocalPlayerActionLabels[i], mode == current) && mode != current)
                {
                    m_SimulationUIStore.SetBotAction(bot.PlayerIndex, mode);
                    this->ApplyBotAction(bot.PlayerIndex, mode);
                }
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    auto SimulationUIService::LogBotAction(std::size_t number, std::size_t playerIndex, const PlayerActions& actions) -> void
    {
        m_LogsService.Message("[SimulationUIService] INFO: Action of bot {} (player {}) at tick {}: in the mask {}, bytes {}.", number,
            playerIndex, actions.Sequence, (actions.Mask & (1u << playerIndex)) != 0 ? "yes" : "no",
            ToHex(actions.Actions[playerIndex].Data));
    }

    auto SimulationUIService::ApplyBotAction(std::size_t playerIndex, LocalPlayerAction mode) -> void
    {
        if (mode == LocalPlayerAction::Engine)
        {
            m_PlayerActionStore.SetOverride(playerIndex, std::nullopt);
            return;
        }

        PlayerActions actions{};
        m_PlayerActionStore.CopyLatest(actions);

        const PlayerAction& action{ actions.Actions[playerIndex] };

        ActionOverride actionOverride{};
        actionOverride.Yaw = ReadAction<float>(action, k_ActionFacingOffset);
        actionOverride.Pitch = ReadAction<float>(action, k_ActionFacingOffset + sizeof(float));

        if (mode == LocalPlayerAction::TestAction)
        {
            actionOverride.ThrottleForward = k_TestThrottle;
            actionOverride.YawPerTick = k_TestYawPerTick;
        }

        m_PlayerActionStore.SetOverride(playerIndex, actionOverride);
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