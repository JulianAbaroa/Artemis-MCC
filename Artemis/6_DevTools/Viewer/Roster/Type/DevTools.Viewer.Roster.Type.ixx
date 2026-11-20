export module DevTools.Viewer.Roster.Type;

import Common.Engine.Type;
import Common.Team.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Viewer::Roster::Type
{
    // A player of the table as the arrow list sees it (VW.9b). The bot number is already resolved, so the ordering needs no bot store.
    struct RosterEntry
    {
        std::uint32_t Handle{ k_NoHandle };
        std::uint32_t BipedHandle{ k_NoHandle };
        std::string Name{};
        Common::Team::Type::Team Team{ Common::Team::Type::Team::Neutral };

        // Number of the Mini-Place bot, empty for a human.
        std::optional<std::size_t> BotNumber{};
        bool IsAlive{ false };
    };

    // The positions the strip shows around the current one. Previous and Next are empty when the list has no other player there.
    struct RosterStrip
    {
        std::optional<std::size_t> Previous{};
        std::size_t Current{ 0 };
        std::optional<std::size_t> Next{};
    };
}