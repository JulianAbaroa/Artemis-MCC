export module Platform.Input.State:Input;

import Platform.Input.Type;
import std;

export namespace Platform::Input::State
{
    // Actions requested by the AI, exposed to the game as held buttons.
    // note: Lock-free. A released action stays held for a minimum number of ticks so the game never misses a short press.
    class InputStore
    {
    private:
        using InputAction = Platform::Input::Type::InputAction;

    public:
        InputStore() = default;
        ~InputStore() = default;

        // Presses or releases an action. Pressing also guarantees the minimum hold time.
        auto SetActionRequested(InputAction action, bool requested) -> void;

        // Advances the hold timers by one Artemis tick.
        auto AdvanceTick() -> void;

        // Whether the engine should see this button as held.
        // param buttonID: Engine button ID.
        // note: Called from the GetButtonState hook, so it must stay lock-free.
        auto IsActionHeld(short buttonID) const -> bool;

        // Releases every action and clears the timers.
        auto Cleanup() -> void;

    private:
        static constexpr std::size_t k_MaxButtonId{ 256 };

        // Per button. Whether the action is currently requested.
        std::array<std::atomic<bool>, k_MaxButtonId> m_Requested{};

        // Per button. Ticks left to keep it held after release.
        std::array<std::atomic<std::uint16_t>, k_MaxButtonId> m_TicksRemaining{};

        static auto ToIndex(short buttonID) -> std::size_t;
        static auto ToIndex(InputAction action) -> std::size_t;
    };
}