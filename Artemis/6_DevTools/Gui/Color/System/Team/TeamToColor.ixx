module;

#include "External/imgui/imgui.h"

export module DevTools.Gui.Color.System:Team;

import Common.Team.Type;
import std;

export namespace DevTools::Gui::Color::System
{
    // Picks the display color of a team.
    class TeamToColor
    {
    private:
        using Team = Common::Team::Type::Team;

    public:
        TeamToColor() = default;
        ~TeamToColor() = default;

        // param alpha: 0 to 1.
        // return: The team color as a float vector. Grey for the teams without a color.
        static auto TeamColorVec4(Team team, float alpha) -> ImVec4;
    };
}