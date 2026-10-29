export module Bridge.Transport.System:Engine;

import Control.Action.Type;
import Bridge.Transport.Type;
import std;

export namespace Bridge::Transport::System
{
    // What the bridge reads and writes in the engine (D215). Implemented by the owner (Mod), the bridge never imports the engine.
    // note: The bridge calls it from its own thread (Update, EndTick), from the AI thread (CaptureClock) and from the game thread
    // (Apply, and BeginMatch when the engine initializes).
    class BridgeEngine
    {
    private:
        using ActionOverride = Control::Action::Type::ActionOverride;
        using ClientRequests = Control::Action::Type::ClientRequests;

        using BridgePhase = Contract::Bridge::Type::BridgePhase;

    public:
        virtual ~BridgeEngine() = default;

        // return: Phase of the engine for the client: loading, running or idle.
        virtual auto GetPhase() const -> BridgePhase = 0;

        // return: Number of times the engine has started, the id of the current match.
        virtual auto GetEngineStarts() const -> std::uint64_t = 0;

        // return: Frames the engine has spent paused.
        virtual auto GetPausedFrames() const -> std::uint64_t = 0;

        virtual auto GetRequestedTimeScale() const -> float = 0;
        virtual auto SetRequestedTimeScale(float scale) -> void = 0;

        virtual auto IsPauseRequested() const -> bool = 0;
        virtual auto SetPauseRequested(bool value) -> void = 0;

        virtual auto SetEndRoundRequested(bool value) -> void = 0;

        // Sets the override of a player for the next action, or none to give the player back to the engine.
        virtual auto SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void = 0;

        // Asks the engine for the weapon, grenade and interaction requests of a client player.
        virtual auto RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void = 0;
    };
}