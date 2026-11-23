export module Engine.Control.Input.State:PlayerAction;

import Control.Action.Type;
import Control.Client.Type;
import Engine.Control.Input.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
    using Engine::Control::Input::Type::k_ShadowFieldCount;
}

export namespace Engine::Control::Input::State
{
    // Player actions of the latest tick, what each action did since the watch was cleared, and the overrides Artemis writes (A9 M4).
    // note: Thread-safe. The PlayerAction hook reads the overrides and publishes the actions on the game thread once per tick.
    // Players are indexed by their player table index.
    class PlayerActionStore
    {
    private:
        using ActionOverride = ::Control::Action::Type::ActionOverride;
        using ClientRequests = ::Control::Action::Type::ClientRequests;

        using PlayerActions = Engine::Control::Input::Type::PlayerActions;
        using ActionWatch = Engine::Control::Input::Type::ActionWatch;
        using ShadowField = Engine::Control::Input::Type::ShadowField;
        using ShadowCount = Engine::Control::Input::Type::ShadowCount;
        using AgentDelivery = Engine::Control::Input::Type::AgentDelivery;

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
        // note: A value takes the AgentInput of the player away (an override and an AgentInput are exclusive, the last one set wins). Empty leaves it.
        auto SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void;

        // Gives the player a window of AgentInput (VW.9c.3 C): the client of Artemis plays it every tick, and when the window runs out it repeats the last
        // one without a new press and without look until the next delivery. The sequence number rises with each call.
        // param input: Sanitized by the client, so the store keeps what it is given.
        // param windowTicks: Ticks of the window (d), at least 1.
        // param turnLimit: Radians per tick and per axis the look may ask, 0 = no limit (not finite or negative is kept as 0).
        // note: Takes the override of the player away (exclusive). Out of range players are ignored.
        auto SetAgentInput(std::size_t playerIndex, const ::Control::Client::Type::AgentInput& input, std::uint32_t windowTicks, float turnLimit) -> void;

        // Stops the AgentInput of the player: the tick after it, the player has the engine action again.
        auto ClearAgentInput(std::size_t playerIndex) -> void;

        // param out: Out. The AgentInput of every player, empty for a player without one.
        auto CopyAgentInputs(std::array<std::optional<AgentDelivery>, k_MaxActionPlayers>& out) const -> void;

        // Copies the overrides and the AgentInputs of every player in one lock, so the hook sees them exclusive in the same tick.
        // param overrides: Out. Override of every player.
        // param deliveries: Out. AgentInput of every player, empty for a player without one.
        auto CopyActions(std::array<std::optional<ActionOverride>, k_MaxActionPlayers>& overrides,
            std::array<std::optional<AgentDelivery>, k_MaxActionPlayers>& deliveries) const -> void;

        // param out: Out. Override of every player.
        auto CopyOverrides(std::array<std::optional<ActionOverride>, k_MaxActionPlayers>& out) const -> void;

        // Adds the radians to the yaw of the override of the player, if it still has one, and keeps it in [0, 2 pi).
        auto AdvanceOverrideYaw(std::size_t playerIndex, float radians) -> void;

        // Adds client requests for the player (D120). They add up with the ones not taken yet.
        auto RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void;

        // Moves the pending client requests of every player out of the store.
        // param out: Out. Requests of every player, empty for a player without one.
        auto TakeClientRequests(std::array<ClientRequests, k_MaxActionPlayers>& out) -> void;

        // Turns the shadow check on or off (D122). Off by default.
        auto SetShadowEnabled(bool isEnabled) -> void;
        auto IsShadowEnabled() const -> bool;

        // Counts one comparison of the shadow check.
        // return: The mismatches of the field so far, this one included.
        auto AddShadow(ShadowField field, bool isMatch) -> std::uint64_t;

        // param out: Out. Comparisons of every field since the last clear.
        auto CopyShadow(std::array<ShadowCount, k_ShadowFieldCount>& out) const -> void;

        // Zeroes the comparisons of the shadow check.
        auto ClearShadow() -> void;

        // Drops the actions, the watches, the overrides, the AgentInputs and the client requests.
        auto Cleanup() -> void;

    private:
        PlayerActions m_Latest{};
        std::array<ActionWatch, k_MaxActionPlayers> m_Watches{};
        std::array<std::optional<ActionOverride>, k_MaxActionPlayers> m_Overrides{};
        std::array<ClientRequests, k_MaxActionPlayers> m_ClientRequests{};
        std::array<std::optional<AgentDelivery>, k_MaxActionPlayers> m_AgentInputs{};
        std::uint64_t m_AgentSequence{ 0 };
        std::array<ShadowCount, k_ShadowFieldCount> m_Shadow{};
        // Atomic: the hooks read it on every call and must not take the mutex while the check is off.
        std::atomic<bool> m_IsShadowEnabled{ false };

        mutable std::mutex m_Mutex{};
    };
}