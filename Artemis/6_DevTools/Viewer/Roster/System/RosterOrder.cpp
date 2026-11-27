module DevTools.Viewer.Roster.System;

namespace
{
    using DevTools::Viewer::Roster::Type::RosterEntry;
    using DevTools::Viewer::Roster::Type::RosterStrip;
    using Perception::Players::Type::Players;

    // return: Whether a goes before b.
    auto Precedes(const RosterEntry& a, const RosterEntry& b) -> bool
    {
        if (a.Team != b.Team) return static_cast<std::uint8_t>(a.Team) < static_cast<std::uint8_t>(b.Team);

        const bool isBotA{ a.BotNumber.has_value() };
        const bool isBotB{ b.BotNumber.has_value() };
        if (isBotA != isBotB) return !isBotA;

        if (isBotA && *a.BotNumber != *b.BotNumber) return *a.BotNumber < *b.BotNumber;
        if (!isBotA && a.Name != b.Name) return a.Name < b.Name;

        return a.Handle < b.Handle;
    }
}

namespace DevTools::Viewer::Roster::System
{
    auto SortRoster(std::vector<RosterEntry>& roster) -> void
    {
        std::ranges::sort(roster, Precedes);
    }

    auto IndexOfPlayer(const std::vector<RosterEntry>& roster, std::uint32_t handle) -> std::optional<std::size_t>
    {
        const auto it{ std::ranges::find(roster, handle, &RosterEntry::Handle) };
        if (it == roster.end()) return std::nullopt;

        return static_cast<std::size_t>(it - roster.begin());
    }

    auto StepFrom(std::size_t count, std::optional<std::size_t> current, int delta) -> std::optional<std::size_t>
    {
        if (count == 0 || delta == 0) return current && *current < count ? current : std::nullopt;

        if (!current || *current >= count) return delta > 0 ? 0 : count - 1;

        const auto size{ static_cast<std::int64_t>(count) };
        const std::int64_t next{ (static_cast<std::int64_t>(*current) + delta) % size };

        return static_cast<std::size_t>(next < 0 ? next + size : next);
    }

    auto DecidePossession(bool isOptionOn, bool isGameMode, std::uint32_t observed, bool isObservedBot) -> std::optional<std::uint32_t>
    {
        if (!isOptionOn || !isGameMode || !isObservedBot || observed == 0xFFFFFFFF) return std::nullopt;

        return observed;
    }

    auto KeepAgents(std::vector<RosterEntry>& roster) -> void
    {
        std::erase_if(roster, [](const RosterEntry& entry) { return !entry.BotNumber.has_value(); });
    }

    auto AgentRowsOf(const std::vector<RosterEntry>& roster, const Players& players) -> std::vector<std::size_t>
    {
        std::vector<std::size_t> rows{};

        for (std::size_t row{ 0 }; row < players.Rows.size(); ++row)
        {
            const std::uint32_t handle{ players.Rows[row].Handle };
            const bool isAgent{ std::ranges::any_of(roster, [handle](const RosterEntry& entry) { return entry.Handle == handle && entry.BotNumber.has_value(); }) };
            if (isAgent) rows.push_back(row);
        }

        return rows;
    }

    auto ShownRowsOf(const std::vector<std::size_t>& agentRows, std::optional<std::size_t> focusRow, std::optional<std::size_t> fallbackRow) -> std::vector<std::size_t>
    {
        if (focusRow) return { *focusRow };
        if (!agentRows.empty()) return agentRows;
        if (fallbackRow) return { *fallbackRow };

        return {};
    }

    auto StripOf(std::size_t count, std::size_t current) -> RosterStrip
    {
        RosterStrip strip{ .Current = current };
        if (count < 2) return strip;

        strip.Next = (current + 1) % count;
        if (count > 2) strip.Previous = (current + count - 1) % count;

        return strip;
    }
}