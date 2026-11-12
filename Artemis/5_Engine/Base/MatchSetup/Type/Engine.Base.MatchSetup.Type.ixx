export module Engine.Base.MatchSetup.Type;

import Common.Engine.Type;
import Engine.Base.MapVariant.Type;
import Engine.Base.Preset.Type;
import Blam.Layout.Game.Type;
import std;

namespace
{
    using Engine::Base::MapVariant::Type::ActiveMapVariant;
    using Engine::Base::Preset::Type::PresetPick;

    using Blam::Layout::Game::Type::Offset::k_RoundStateBetween;
    using Blam::Layout::Game::Type::Offset::k_RoundStateEnded;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxTeams;
}

export namespace Engine::Base::MatchSetup::Type
{
    // Machines the engine tracks (16 players and the host).
    inline constexpr std::size_t k_MaxMachines{ 17 };

    // Players one machine can have: the user index and the controller (0 to 3) cannot repeat inside a machine.
    inline constexpr std::size_t k_MaxPlayersPerMachine{ 4 };

    // Bots one instance can hold: 4 per fake machine, on every machine slot except the first one (the local machine). The player table
    // (k_MaxPlayers) is the limit that binds first. Here and not in Engine.Control because the setup checks it before any match runs.
    inline constexpr std::size_t k_MaxBots{ (k_MaxMachines - 1) * k_MaxPlayersPerMachine };

    // The bots a match should have (D284): how many on each team, and how many more on the team of the local player 0, which is
    // resolved when the roster is applied. The local players are not part of it.
    struct Roster
    {
        std::array<std::size_t, k_MaxTeams> Teams{};
        std::size_t Auto{ 0 };

        auto operator==(const Roster&) const -> bool = default;
    };

    // Who asked for a setup. A client that is gone (Python lost its instance) cancels only what it asked for.
    enum class MatchSetupOrigin : std::uint8_t
    {
        Bridge,
        Bench,
        Ui,
    };

    // What a match gets in one step (D286): the gametype preset, the map variant and the bots, with their values already resolved from
    // names by the thread that asks. A field that is not set means no change.
    struct MatchSetup
    {
        // A pick with no preset asks for the base values of the gametype.
        std::optional<PresetPick> Preset{};
        // A null map variant asks for the one the match loaded with.
        std::optional<std::shared_ptr<const ActiveMapVariant>> Map{};
        std::optional<Roster> Bots{};
    };

    // What became of the last setup request (D286).
    enum class MatchSetupOutcome : std::uint8_t
    {
        // No request yet.
        None,
        // Waiting for the next start or for the end of a round.
        Pending,
        // Applied at the start of the match.
        AtLoad,
        // Applied between two rounds.
        Applied,
        // Not applied, or only partly: see Reason.
        Rejected,
    };

    // Result of the last setup request and the round of the match it was applied in.
    struct MatchSetupResult
    {
        MatchSetupOutcome Outcome{ MatchSetupOutcome::None };
        // Number of the request it belongs to.
        std::uint64_t Sequence{ 0 };
        // Round index of the match when it was applied, or -1.
        std::int16_t Round{ -1 };
        std::string Reason{};

        auto operator==(const MatchSetupResult&) const -> bool = default;
    };

    // A setup asked for, with the number of the request. The result of an older request never replaces the one of a newer request.
    struct MatchSetupRequest
    {
        MatchSetup Value{};
        std::uint64_t Sequence{ 0 };
        MatchSetupOrigin Origin{ MatchSetupOrigin::Bench };
    };

    // What the start of a match leaves for its first tick: the bots to add, which the game thread can only do once the match runs.
    struct MatchSetupLoad
    {
        std::uint64_t Sequence{ 0 };
        // Empty when the start applied everything or refused the setup, or when the roster is the active one and no request is waiting for
        // its result.
        std::optional<Roster> Bots{};
        // Whether the start wrote a preset or a map variant into the configuration: a roster that fails at the first tick is then a partial
        // result, never a Rejected one.
        bool IsWritten{ false };
    };

    // The setup the engine holds, which every start applies again: the names of the preset and of the map variant ("" for none) and the
    // bots.
    struct ActiveMatchSetup
    {
        std::string Preset{};
        std::string Map{};
        std::optional<Roster> Bots{};
    };

    // What the bots service did with a roster: operations done of the total of the plan, and why nothing was done when it did not fit.
    struct RosterApplication
    {
        std::size_t Done{ 0 };
        std::size_t Total{ 0 };
        std::string Reason{};
    };

    // What the setup service does on a tick.
    enum class MatchSetupStep : std::uint8_t
    {
        Nothing,
        // First tick of the match: the bots of the start.
        Load,
        // A request waits for the end of the round, and this is a tick of the break.
        Break,
    };

    // return: What the setup service does on this tick. A request is applied once, in a tick where the round is ended or between rounds,
    // never in the middle of a round.
    // param roundState: Round state of the game engine globals, empty if it could not be read.
    auto GetMatchSetupStep(bool isMatchStarted, bool hasPending, std::optional<std::uint8_t> roundState) -> MatchSetupStep
    {
        if (!isMatchStarted) return MatchSetupStep::Load;
        if (!hasPending || !roundState) return MatchSetupStep::Nothing;

        return (*roundState == k_RoundStateEnded || *roundState == k_RoundStateBetween) ? MatchSetupStep::Break : MatchSetupStep::Nothing;
    }

    // return: The roster as text for logs and the screens: the teams with bots as "team:count" and the auto ones, or "empty".
    auto ToText(const Roster& roster) -> std::string
    {
        std::string text{};

        for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
        {
            if (roster.Teams[team] > 0) text += std::format("{}{}:{}", text.empty() ? "" : " ", team, roster.Teams[team]);
        }

        if (roster.Auto > 0) text += std::format("{}auto:{}", text.empty() ? "" : " ", roster.Auto);

        return text.empty() ? "empty" : text;
    }

    // return: The setup the engine holds and what became of the last request, in one line for the Workbench and the bench report.
    auto ToText(const ActiveMatchSetup& active, const MatchSetupResult& result) -> std::string
    {
        std::string state{};

        switch (result.Outcome)
        {
        case MatchSetupOutcome::None: state = "applies at the next load"; break;
        case MatchSetupOutcome::Pending: state = "pending"; break;
        case MatchSetupOutcome::AtLoad: state = result.Reason.empty() ? std::string{ "at load" } : "at load, " + result.Reason; break;
        case MatchSetupOutcome::Applied:
            state = std::format("between rounds {}", result.Round) + (result.Reason.empty() ? std::string{} : ", " + result.Reason);
            break;
        case MatchSetupOutcome::Rejected: state = "rejected: " + result.Reason; break;
        }

        return std::format("Setup: preset {}, map {}, roster {} ({})", active.Preset.empty() ? std::string{ "none" } : active.Preset,
            active.Map.empty() ? std::string{ "none" } : active.Map, active.Bots ? ToText(*active.Bots) : std::string{ "none" }, state);
    }

    // return: Why that many bots do not fit: more than k_MaxBots, or more than the free slots of the player table. Nothing if they fit.
    // note: The one place that decides it: the bots service plans with it (PlanRoster) and the setup checks with it before any match.
    auto CheckRosterFit(std::size_t bots, std::size_t otherPlayers) -> std::optional<std::string>
    {
        if (bots > k_MaxBots) return std::format("{} bots, at most {}", bots, k_MaxBots);

        if (otherPlayers + bots > k_MaxPlayers)
        {
            return std::format("{} bots and {} other players do not fit in {} player slots", bots, otherPlayers, k_MaxPlayers);
        }

        return std::nullopt;
    }

    // return: Why a roster can never fit a match: its bots and the local player 0 do not fit. Nothing if it can.
    // note: The service that applies it checks the table of the moment (other players). This is what can be known before.
    auto CheckRosterSize(const Roster& roster) -> std::optional<std::string>
    {
        return CheckRosterFit(std::accumulate(roster.Teams.begin(), roster.Teams.end(), roster.Auto), 1);
    }
}