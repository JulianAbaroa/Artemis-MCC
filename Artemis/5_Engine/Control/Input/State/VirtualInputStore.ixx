export module Platform.Input.State:VirtualInput;

import Common.Engine.Type;
import Platform.Input.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxLocalUsers;
}

export namespace Platform::Input::State
{
    // Input of each local player as the engine receives it from MCC once per frame: who provides it and, for virtual players, the state.
    // note: Thread-safe. Written by the UI (later the AI), read by the InputUpdate hook on the game thread.
    class VirtualInputStore
    {
    private:
        using InputState = Platform::Input::Type::InputState;
        using LocalInputSource = Platform::Input::Type::LocalInputSource;

    public:
        VirtualInputStore() = default;
        ~VirtualInputStore() = default;

        VirtualInputStore(const VirtualInputStore&) = delete;
        auto operator=(const VirtualInputStore&) -> VirtualInputStore& = delete;

        // note: Every player starts with LocalInputSource::Device, so the game is untouched until a player is switched.
        auto SetSource(std::size_t player, LocalInputSource source) -> void;
        auto GetSource(std::size_t player) const -> LocalInputSource;

        // Sets the state delivered to the player on every frame while the source is Virtual.
        // note: Mouse deltas are delivered on every frame too (a held movement), they are not consumed.
        auto SetState(std::size_t player, const InputState& state) -> void;

        // param out: Out. State of the player.
        auto CopyState(std::size_t player, InputState& out) const -> void;

        // Sets every player back to Device with an empty state.
        auto Cleanup() -> void;

    private:
        std::array<std::atomic<LocalInputSource>, k_MaxLocalUsers> m_Sources{};
        std::array<InputState, k_MaxLocalUsers> m_States{};

        mutable std::mutex m_StateMutex{};
    };
}