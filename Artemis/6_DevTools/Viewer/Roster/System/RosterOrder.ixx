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

    // return: The previous, current and next positions of the strip. One player has no neighbours. With two, the other one is shown only as the next.
    auto StripOf(std::size_t count, std::size_t current) -> Roster::Type::RosterStrip;
}