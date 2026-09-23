export module UI.Format.System:Team;

import Common.Team.Type;

export namespace UI::Format::System
{
	class TeamFormater
	{
	private:
		using Team = Common::Team::Type::Team;

	public:
		TeamFormater() = default;
		~TeamFormater() = default;

		static auto TeamToString(Team team) -> const char*;
	};
}