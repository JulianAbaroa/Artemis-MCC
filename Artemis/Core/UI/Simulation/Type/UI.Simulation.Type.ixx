export module UI.Simulation.Type;

import std;

export namespace UI::Simulation::Type
{
    // Input the simulation tab gives a local player (A9 M2 test).
    enum class LocalPlayerInput : std::uint8_t
    {
        // MCC reads the real devices.
        Device = 0,
        // Virtual input with nothing pressed.
        Idle,
        // Virtual keyboard and mouse: W held and the mouse turning right.
        TestKeyboardMouse,
        // Virtual gamepad: left stick forward and right stick turning right.
        TestGamepad,

        Count
    };

    inline constexpr std::size_t k_LocalPlayerInputCount{ static_cast<std::size_t>(LocalPlayerInput::Count) };

    // Combo labels, in the order of LocalPlayerInput.
    inline constexpr std::array<const char*, k_LocalPlayerInputCount> k_LocalPlayerInputLabels
    {
        "Device (MCC)",
        "Idle",
        "Test keyboard and mouse",
        "Test gamepad",
    };

    // Action the simulation tab writes for a local player or a bot every tick (A9 M4b and M5b, D109).
    enum class LocalPlayerAction : std::uint8_t
    {
        // The action the engine built from the input.
        Engine = 0,
        // Nothing pressed, no throttle, and the facing it had when picked.
        HoldStill,
        // Full throttle forward and the yaw turning one degree per tick.
        TestAction,
        // Hold still and keep pressing use. The interaction comes from the engine functions, for bots too (A9 M5d, D118).
        TestUse,
        // Hold still and hold use without releasing it (D121: the held bit appears at the hold threshold of the engine).
        HoldUse,

        Count
    };

    inline constexpr std::size_t k_LocalPlayerActionCount{ static_cast<std::size_t>(LocalPlayerAction::Count) };

    // Radio labels, in the order of LocalPlayerAction.
    inline constexpr std::array<const char*, k_LocalPlayerActionCount> k_LocalPlayerActionLabels
    {
        "Engine",
        "Hold still",
        "Test action",
        "Test use",
        "Hold use",
    };
}