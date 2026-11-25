export module DevTools.Viewer.Roster.System;

import Common.Engine.Type;
import DevTools.Viewer.Roster.Type;
import std;

export namespace DevTools::Viewer::Roster::System
{
    // Sorts the list by team (the enum value, so Red first and Neutral last), then the humans by name, then the bots by number. Ties go by handle.
    auto SortRoster(std::vector<Roster::Type::RosterEntry>& roster) -> void;

    // return: The position of the player with that handle, or nothing when it is not in the list.
    auto IndexOfPlayer(const std::vector<Roster::Type::RosterEntry>& roster, std::uint32_t handle) -> std::optional<std::size_t>;

    // return: The position reached from the current one by delta steps, wrapping at both ends. Without a current position, a positive delta gives the
    // first and a negative one the last. Nothing for an empty list or a zero delta without a current position.
    auto StepFrom(std::size_t count, std::optional<std::size_t> current, int delta) -> std::optional<std::size_t>;

    // return: The player the game must let the user possess, or nothing (VW.9c.2 B, D352). Only a bot, only with the option on and the Game camera:
    // the Free camera, a human selected, no player selected or the option off give nothing, which releases the bot. The caller runs it only while the
    // Viewer is active (an inactive Viewer withdraws the request in SceneService::Deactivate).
    // param observed: The player observed (the selected one), k_NoHandle when none.
    // param isObservedBot: The observed player is an Artemis bot of the roster. It stays true while the bot is dead: the selection follows the player.
    auto DecidePossession(bool isOptionOn, bool isGameMode, std::uint32_t observed, bool isObservedBot) -> std::optional<std::uint32_t>;

    // Drops the rows that are not Artemis bots (no bot number), keeping the order of the rest.
    auto KeepAgents(std::vector<Roster::Type::RosterEntry>& roster) -> void;

    // return: The previous, current and next positions of the strip. One player has no neighbours. With two, the other one is shown only as the next.
    auto StripOf(std::size_t count, std::size_t current) -> Roster::Type::RosterStrip;
}