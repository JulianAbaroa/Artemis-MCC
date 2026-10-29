export module DevTools.Gui.Format.System:Team;

import Common.Team.Type;

export namespace DevTools::Gui::Format::System
{
    // Converts the team enum to display text. Unlisted values give "Unknown".
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