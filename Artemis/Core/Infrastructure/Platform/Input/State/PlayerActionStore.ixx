export module Platform.Input.State:PlayerAction;

import Platform.Input.Type;
import std;

namespace
{
    using Platform::Input::Type::k_MaxActionPlayers;
}

export namespace Platform::Input::State
{
    // Player actions of the latest tick, what each action did since the watch was cleared, and the overrides Artemis writes (A9 M4).
    // note: Thread-safe. The PlayerAction hook reads the overrides and publishes the actions on the game thread once per tick.
    // Players are indexed by their player table index.
    class PlayerActionStore
    {
    private:
        using PlayerActions = Platform::Input::Type::PlayerActions;
        using ActionOverride = Platform::Input::Type::ActionOverride;
        using ActionWatch = Platform::Input::Type::ActionWatch;
        using ClientRequests = Platform::Input::Type::ClientRequests;

    public:
        PlayerActionStore() = default;
        ~PlayerActionStore() = default;

        PlayerActionStore(const PlayerActionStore&) = delete;
        auto operator=(const PlayerActionStore&) -> PlayerActionStore& = delete;

        // Replaces the latest actions and adds them to the watch of every player with an action.
        auto Publish(const PlayerActions& actions) -> void;

        // param out: Out. Actions of the latest tick.
        auto CopyLatest(PlayerActions& out) const -> void;

        // param out: Out. Watch of the player.
        auto CopyWatch(std::size_t playerIndex, ActionWatch& out) const -> void;

        // Clears every watch. The next action of each player becomes its reference.
        auto ClearWatches() -> void;

        // Sets the fields Artemis writes into the action of the player every tick, or empty to leave the engine action.
        auto SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void;

        // param out: Out. Override of every player.
        auto CopyOverrides(std::array<std::optional<ActionOverride>, k_MaxActionPlayers>& out) const -> void;

        // Adds the radians to the yaw of the override of the player, if it still has one, and keeps it in [0, 2 pi).
        auto AdvanceOverrideYaw(std::size_t playerIndex, float radians) -> void;

        // Adds client requests for the player (D120). They add up with the ones not taken yet.
        auto RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void;

        // Moves the pending client requests of every player out of the store.
        // param out: Out. Requests of every player, empty for a player without one.
        auto TakeClientRequests(std::array<ClientRequests, k_MaxActionPlayers>& out) -> void;

        // Drops the actions, the watches, the overrides and the client requests.
        auto Cleanup() -> void;

    private:
        PlayerActions m_Latest{};
        std::array<ActionWatch, k_MaxActionPlayers> m_Watches{};
        std::array<std::optional<ActionOverride>, k_MaxActionPlayers> m_Overrides{};
        std::array<ClientRequests, k_MaxActionPlayers> m_ClientRequests{};

        mutable std::mutex m_Mutex{};
    };
}