export module Platform.Simulation.System;

import Common.Log.System;
import Platform.Memory.System;
import Platform.Simulation.Type;
import Platform.Simulation.State;
import std;

export namespace Platform::Simulation::System
{
    // Adds bots to the match: each one is a player of a machine that only Artemis knows, created like a player that joins in progress
    // (A9 M5b). Also publishes the machines and players of the engine every tick (A9 M5a).
    // note: The simulation sees a bot as a remote player: no view, no local input. Its action of each tick comes from the overrides of
    // the PlayerAction hook (D109). The bots skip the session and the simulation queue, so the film does not have them (A9 sec. 13.4).
    class BotService
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Platform::Memory::System::AOBService;

        using SetMachinesFunction = Platform::Simulation::Type::SetMachinesFunction;
        using ApplyPlayerAddedFunction = Platform::Simulation::Type::ApplyPlayerAddedFunction;
        using ApplyPlayerConfigurationFunction = Platform::Simulation::Type::ApplyPlayerConfigurationFunction;
        using GatherInteractionsFunction = Platform::Simulation::Type::GatherInteractionsFunction;
        using UnitSelectorFunction = Platform::Simulation::Type::UnitSelectorFunction;
        using NextZoomLevelFunction = Platform::Simulation::Type::NextZoomLevelFunction;
        using CanUnitZoomFunction = Platform::Simulation::Type::CanUnitZoomFunction;
        using UnitClientState = Platform::Simulation::Type::UnitClientState;
        using TeamRequest = Platform::Simulation::Type::TeamRequest;

        using BotStore = Platform::Simulation::State::BotStore;

    public:
        BotService(Log& log, AOBService& aobService, BotStore& botStore) :
            m_Log(log), m_AOBService(aobService), m_BotStore(botStore) {}
        ~BotService() = default;

        BotService(const BotService&) = delete;
        auto operator=(const BotService&) -> BotService& = delete;

        // Publishes the session snapshot, adds the bots asked for since the last tick and applies their team changes.
        // param threadContext: Thread context of the engine, or 0 if unknown (then it does nothing).
        // note: Game thread only, before the player actions of the tick are applied.
        auto Update(std::uintptr_t threadContext) -> void;

        // return: The action slot of the unit of the player (unit +0x288), or nullopt if the player has no live unit.
        // note: Game thread only. A bot needs it in its action so the engine moves its unit (A9 sec. 13.10).
        auto GetUnitActionSlot(std::uintptr_t threadContext, std::size_t playerIndex) const -> std::optional<std::uint8_t>;

        // Writes the interaction of the player with the engine functions that build it for a local player (0x4f280), so every agent,
        // local or bot, gets the interaction of the engine at every tick (A9 M5d, D118).
        // param interaction: k_InteractionRecordCount records (the interaction of the action of the player).
        // return: False if the functions were not found or the player is gone. The interaction is then left as it was.
        // note: Game thread only. The interaction comes from the unit (its position and its weapon), not from a view. Without a unit
        // it is empty, as the engine leaves it. The call 0x4f280 makes to tell MCC that the prompt changed (/enableSingleInteract) is
        // skipped: it belongs to the HUD of the local user, not to this player.
        auto WriteInteraction(std::uintptr_t threadContext, std::size_t playerIndex, std::span<std::uint8_t> interaction) const
            -> bool;

        // return: The client state of the unit of the player, or nullopt if the player is gone or has no live unit.
        // note: Game thread only.
        auto GetUnitClientState(std::uintptr_t threadContext, std::size_t playerIndex) const -> std::optional<UnitClientState>;

        // return: The slot the engine picks when the player switches weapon (0x48b618 from the slot in the state, as a local client
        // asks for it), or nullopt if it has no other weapon, sits in a seat or the functions were not found (D120).
        // note: Game thread only. A local client only switches in a seat whose tag allows it (seat flag bit 5, 0x61a38). That flag is
        // not read yet, so a seated unit never asks (v1 has no vehicles, D115).
        auto GetNextWeaponSlot(const UnitClientState& state) const -> std::optional<std::int8_t>;

        // return: The next grenade type with grenades left (0x48b58c), or nullopt if there is no other one (D120).
        // note: Game thread only.
        auto GetNextGrenadeType(const UnitClientState& state) const -> std::optional<std::int8_t>;

        // return: The next zoom level of the weapon in the hands (0x485838, -1 = not zoomed), or nullopt if the unit cannot zoom
        // now (0x4852a0) (D120).
        // note: Game thread only.
        auto GetNextZoomLevel(const UnitClientState& state) const -> std::optional<std::int8_t>;

        // Finds the engine functions by signature, in three sets: the ones that add bots, the ones that build the interaction and the
        // client selectors (weapon, grenade, zoom).
        // return: False if one is not found. The cause is logged. Without the first set no bot can be added, without the second the
        // agents keep the interaction the engine built (none for a bot), without the third the agents cannot switch weapon,
        // grenade or zoom.
        // note: Call it before the SetMachines hook is installed: the hook overwrites the bytes the signature matches.
        auto ResolveFunctions() -> bool;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        BotStore& m_BotStore;

        // Ticks seen. Game thread only.
        std::uint64_t m_Sequence{ 0 };

        // Engine functions. The SetMachines call goes through its hook, which keeps the fake machines.
        SetMachinesFunction m_SetMachines{ nullptr };
        ApplyPlayerAddedFunction m_ApplyPlayerAdded{ nullptr };
        ApplyPlayerConfigurationFunction m_ApplyPlayerConfiguration{ nullptr };
        bool m_IsResolved{ false };

        GatherInteractionsFunction m_GatherNearbyInteractions{ nullptr };
        GatherInteractionsFunction m_GatherAimedInteractions{ nullptr };
        bool m_IsInteractionResolved{ false };

        UnitSelectorFunction m_NextWeaponSlot{ nullptr };
        UnitSelectorFunction m_NextGrenadeType{ nullptr };
        NextZoomLevelFunction m_NextZoomLevel{ nullptr };
        CanUnitZoomFunction m_CanUnitZoom{ nullptr };
        bool m_IsClientResolved{ false };

        // Reads the machines, the players and the game options of the engine into the store.
        auto PublishSnapshot(std::uintptr_t threadContext) -> void;

        // Creates the machine of the next bot if needed, then the bot.
        // param team: Team in team games, or nullopt for the team of the local player.
        // return: False if the engine refused it. The cause is logged.
        auto AddBot(std::uintptr_t threadContext, std::optional<std::int8_t> team) -> bool;

        // Moves a player to another team through its configuration, as a team change in the game does (the unit dies and respawns).
        auto ApplyTeam(std::uintptr_t threadContext, const TeamRequest& request) -> void;

        // Makes sure the fake machine exists in the engine.
        // return: False if the slot belongs to a real machine or the engine did not take it.
        auto EnsureFakeMachine(std::uintptr_t playersGlobals, std::size_t machineIndex) -> bool;
    };
}