export module Platform.Input.State:PlayerAction;

import Platform.Input.Type;
import std;

namespace
{
    using Platform::Input::Type::k_MaxActionPlayers;
}

export namespace Platform::Input::State
{
    // Player actions of the latest tick, and the buttons each player held at any tick since they were cleared (A9 M4a).
    // note: Thread-safe. Written by the PlayerAction hook on the game thread once per tick, read by the UI.
    class PlayerActionStore
    {
    private:
        using PlayerActions = Platform::Input::Type::PlayerActions;

    public:
        PlayerActionStore() = default;
        ~PlayerActionStore() = default;

        PlayerActionStore(const PlayerActionStore&) = delete;
        auto operator=(const PlayerActionStore&) -> PlayerActionStore& = delete;

        // Replaces the latest actions and adds their buttons to the buttons seen.
        auto Publish(const PlayerActions& actions) -> void;

        // param out: Out. Actions of the latest tick.
        auto CopyLatest(PlayerActions& out) const -> void;

        // return: Every button bit the player held at some tick since the last ClearSeenButtons.
        // param playerIndex: Index of the player in the player table.
        auto GetSeenButtons(std::size_t playerIndex) const -> std::uint64_t;

        auto ClearSeenButtons() -> void;

        // Drops the actions and the buttons seen.
        auto Cleanup() -> void;

    private:
        PlayerActions m_Latest{};
        std::array<std::uint64_t, k_MaxActionPlayers> m_SeenButtons{};

        mutable std::mutex m_Mutex{};
    };
}