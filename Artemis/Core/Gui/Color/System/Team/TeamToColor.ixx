module;

#include "External/imgui/imgui.h"

export module Gui.Color.System:Team;

import Common.Team.Type;
import std;

export namespace Gui::Color::System
{
    // Picks the display color of a team.
    class TeamToColor
    {
    private:
        using Team = Common::Team::Type::Team;

    public:
        TeamToColor() = default;
        ~TeamToColor() = default;

        // param alpha: 0 to 255.
        // return: The team color packed for the ImGui draw lists. Grey for the teams without a color.
        static auto TeamColorU32(Team team, std::uint8_t alpha) -> ImU32;

        // param alpha: 0 to 1.
        // return: The same color as TeamColorU32, as a float vector.
        static auto TeamColorVec4(Team team, float alpha) -> ImVec4;
    };
}