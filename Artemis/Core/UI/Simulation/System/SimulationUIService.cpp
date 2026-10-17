module;

#include "External/imgui/imgui.h"

module UI.Simulation.System;

import Platform.Host.Type;
import std;

namespace
{
    using Platform::Input::Type::LocalInputSource;

    using Platform::Host::Type::k_MaxLocalUsers;
    using UI::Simulation::Type::k_LocalPlayerInputCount;
    using UI::Simulation::Type::k_LocalPlayerInputLabels;

    // Virtual-key code of W. The engine button W (32) reads it through its key table.
    constexpr std::size_t k_ForwardKey{ 0x57 };

    // Mouse movement delivered on every frame by the keyboard and mouse test, in mouse counts.
    constexpr float k_TestMouseDelta{ 1.0f };

    // Stick values of the gamepad test: full forward on the left stick and a steady turn on the right stick.
    constexpr std::int16_t k_StickForward{ 32767 };
    constexpr std::int16_t k_TestStickTurn{ 12000 };

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
        ImGui::SeparatorText("Local players");

        this->DrawLocalPlayers();
    }

    auto SimulationUIService::DrawLocalPlayers() -> void
    {
        ImGui::TextWrapped("Split-screen users Artemis adds next to yours. The count applies when the next match starts.");

        int count{ static_cast<int>(m_LocalUserStore.GetRequestedCount()) };

        if (ImGui::SliderInt("Players", &count, 1, static_cast<int>(k_MaxLocalUsers)))
        {
            m_LocalUserStore.SetRequestedCount(static_cast<std::size_t>(count));
        }

        const ImGuiTableFlags flags{ ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg };

        if (!ImGui::BeginTable("LocalPlayers", 4, flags)) return;

        ImGui::TableSetupColumn("Player", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Engine");
        ImGui::TableSetupColumn("Input");
        ImGui::TableHeadersRow();

        const std::size_t requestedCount{ m_LocalUserStore.GetRequestedCount() };

        for (std::size_t player{ 0 }; player < k_MaxLocalUsers; ++player)
        {
            this->DrawLocalPlayerRow(player, requestedCount);
        }

        ImGui::EndTable();
    }

    auto SimulationUIService::DrawLocalPlayerRow(std::size_t player, std::size_t requestedCount) -> void
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