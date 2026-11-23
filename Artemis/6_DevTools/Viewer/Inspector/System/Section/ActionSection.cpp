module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Action;

import Common.Engine.Type;
import Control.Action.Type;
import Engine.Control.Input.Type;
import Engine.Control.Simulation.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Control::Action::Type::ActionOverride;
    using Engine::Control::Input::Type::AgentDelivery;
    using Engine::Control::Input::Type::PlayerActions;
    using Engine::Control::Input::Type::ReadAction;
    using Engine::Control::Simulation::Type::Bot;

    using Common::Engine::Type::k_HandleIndexMask;
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
    using Blam::Layout::Action::Type::Offset::k_ActionButtonsOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionFacingOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionThrottleOffset;

    constexpr float k_RadiansToDegrees{ 180.0f / std::numbers::pi_v<float> };

    // return: The numbers of the bits that are set, separated by commas, or "none".
    auto DownButtons(std::uint64_t buttons) -> std::string
    {
        std::string text{};

        for (std::size_t bit{ 0 }; bit < std::numeric_limits<std::uint64_t>::digits; ++bit)
        {
            if ((buttons & (std::uint64_t{ 1 } << bit)) == 0) continue;

            if (!text.empty()) text += ", ";
            text += std::to_string(bit);
        }

        return text.empty() ? std::string{ "none" } : text;
    }
}

namespace DevTools::Viewer::Inspector::System
{
    auto ActionSection::Draw(const Player& player, const PlayerActionStore& actionStore, const BotStore& botStore) -> void
    {
        const std::size_t index{ player.Handle & k_HandleIndexMask };
        if (index >= k_MaxActionPlayers)
        {
            ImGui::TextDisabled("No action slot for this player.");
            return;
        }

        PlayerActions actions{};
        actionStore.CopyLatest(actions);

        if (actions.Sequence == 0)
        {
            ImGui::TextDisabled("No tick seen yet.");
            return;
        }

        std::array<std::optional<ActionOverride>, k_MaxActionPlayers> overrides{};
        actionStore.CopyOverrides(overrides);

        std::array<std::optional<AgentDelivery>, k_MaxActionPlayers> deliveries{};
        actionStore.CopyAgentInputs(deliveries);

        std::vector<Bot> bots{};
        botStore.CopyBots(bots);

        const bool isBot{ std::ranges::any_of(bots, [index](const Bot& bot) { return bot.PlayerIndex == index; }) };
        const char* writer{ overrides[index] ? "override (bench / viewer)" : (deliveries[index] ? "AgentInput (bridge / bench)" : (isBot ? "bot" : "engine")) };

        const auto& action{ actions.Actions[index] };
        const bool hasAction{ (actions.Mask & (1u << index)) != 0 };

        ImGui::Text("Written by: %s", writer);
        ImGui::Text("Table index %zu, tick %llu, action this tick: %s", index, static_cast<unsigned long long>(actions.Sequence),
            hasAction ? "yes" : "no");

        const std::uint64_t buttons{ ReadAction<std::uint64_t>(action, k_ActionButtonsOffset) };
        ImGui::Text("Buttons: %016llX", static_cast<unsigned long long>(buttons));
        ImGui::TextWrapped("Down bits: %s", DownButtons(buttons).c_str());

        const float yaw{ ReadAction<float>(action, k_ActionFacingOffset) };
        const float pitch{ ReadAction<float>(action, k_ActionFacingOffset + sizeof(float)) };
        ImGui::Text("Aim: yaw %.4f (%.1f deg), pitch %.4f (%.1f deg)", yaw, yaw * k_RadiansToDegrees, pitch, pitch * k_RadiansToDegrees);

        const float throttleForward{ ReadAction<float>(action, k_ActionThrottleOffset) };
        const float throttleLeft{ ReadAction<float>(action, k_ActionThrottleOffset + sizeof(float)) };
        ImGui::Text("Throttle: forward %.2f, left %.2f", throttleForward, throttleLeft);
    }
}