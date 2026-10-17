module;

#include "External/imgui/imgui.h"

module UI.Simulation.System;

import Platform.Host.Type;
import std;

namespace
{
    using Platform::Input::Type::LocalInputSource;
    using Export::Tick::Type::Player;
    using Export::Tick::Type::Players;

    using Platform::Host::Type::k_MaxLocalUsers;
    using UI::Simulation::Type::k_LocalPlayerInputCount;
    using UI::Simulation::Type::k_LocalPlayerInputLabels;

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