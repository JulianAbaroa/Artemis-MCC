export module UI.Hotkey.Type;

import std;

export namespace UI::Hotkey::Type
{
    // Action a hotkey triggers in the UI.
    enum class Action : std::uint8_t
    {
        None = 0,

        ToggleLauncher,
        ToggleOverlay,
        ToggleMenu,
        ResetMenu,
        LockMenu,
    };

    // Layer that handles the hotkey.
    enum class Scope : std::uint8_t
    {
        UI,
        Viewer,
    };

    // Hotkey definition with the texts the settings window shows.
    struct Binding
    {
        Action Action{ Action::None };
        Scope Scope{ Scope::UI };

        std::uint32_t VirtualKey{ 0 };
        bool RequiresShift{ false };

        const char* Label{ "" };
        const char* Keys{ "" };
        const char* Tooltip{ "" };
    };
}