module;

#include "External/imgui/imgui.h"

export module UI.Color.System:Team;

import Common.Team.Type;
import std;

export namespace UI::Color::System
{
	class TeamToColor
	{
	private:
		using Team = Common::Team::Type::Team;

	public:
		TeamToColor() = default;
		~TeamToColor() = default;

		static auto TeamColorU32(Team team, std::uint8_t alpha) -> ImU32;
		static auto TeamColorVec4(Team team, float alpha) -> ImVec4;
	};
}