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
}