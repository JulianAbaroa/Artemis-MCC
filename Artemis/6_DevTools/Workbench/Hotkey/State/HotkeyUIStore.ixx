export module DevTools.Workbench.Hotkey.State;

import DevTools.Workbench.Hotkey.Type;
import std;

export namespace DevTools::Workbench::Hotkey::State
{
    // Table of the hotkeys the application listens to.
    class HotkeyUIStore
    {
    private:
        using Action = DevTools::Workbench::Hotkey::Type::Action;
        using Scope = DevTools::Workbench::Hotkey::Type::Scope;
        using Binding = DevTools::Workbench::Hotkey::Type::Binding;

    public:
        HotkeyUIStore() = default;
        ~HotkeyUIStore() = default;

        HotkeyUIStore(const HotkeyUIStore&) = delete;
        auto operator=(const HotkeyUIStore&) -> HotkeyUIStore& = delete;

        // Returns all the bindings, including the ones handled by the Viewer.
        auto GetBindings() const -> std::span<const Binding>;

        // Finds the UI binding of a key.
        // return: The binding or nullptr when the key has none.
        auto Find(std::uint32_t virtualKey, bool isShiftDown) const -> const Binding*;

    private:
        static constexpr std::uint32_t k_VkTab{ 0x09 };
        static constexpr std::uint32_t k_VkUp{ 0x26 };
        static constexpr std::uint32_t k_VkDown{ 0x28 };
        static constexpr std::uint32_t k_VkF2{ 0x71 };
        static constexpr std::uint32_t k_VkF3{ 0x72 };
        static constexpr std::uint32_t k_VkF4{ 0x73 };
        static constexpr std::uint32_t k_VkF6{ 0x75 };

        const std::array<Binding, 18> m_Bindings
        { {
            { Action::ToggleLauncher, Scope::UI, k_VkF2, false,
                "Toggle Launcher", "F2", "Show or hide the launcher." },

            { Action::ToggleOverlay, Scope::UI, k_VkF3, false,
                "Toggle Overlay", "F3", "Show or hide the Overlay." },

            { Action::None, Scope::Viewer, k_VkF4, false,
                "Toggle Viewer", "F4", "Show or hide the Viewer." },

            { Action::None, Scope::Viewer, k_VkTab, false,
                "Switch Viewer Mode", "TAB", "Alternates between the free camera and the game camera while the Viewer is active." },

            { Action::None, Scope::Viewer, k_VkUp, false,
                "Next Player", "UP ARROW", "Selects the next player of the roster (by team, humans by name, then bots by number) while the Viewer is active." },

            { Action::None, Scope::Viewer, k_VkDown, false,
                "Previous Player", "DOWN ARROW", "Selects the previous player of the roster (by team, humans by name, then bots by number) while the Viewer is active." },

            { Action::None, Scope::Viewer, k_VkF6, false,
                "Toggle Labels", "F6", "Show or hide the 3D labels over the map." },

            { Action::None, Scope::Viewer, 0, false,
                "Director Play / Stop", "F7", "Plays the saved shots of the map in real time, or stops. Needs the Viewer active." },

            { Action::None, Scope::Viewer, 0, false,
                "Director Add Fixed Shot", "INSERT", "Free camera. Saves the pose of the camera as a fixed shot of the director." },

            { Action::None, Scope::Viewer, 0, false,
                "Director Next Shot", "RIGHT ARROW", "While the director plays, goes to the next shot." },

            { Action::None, Scope::Viewer, 0, false,
                "Director Previous Shot", "LEFT ARROW", "While the director plays, goes to the previous shot." },

            { Action::ToggleMenu, Scope::UI, '1', true,
                "Toggle Menu", "SHIFT + 1", "Show or hide the Artemis Control Panel." },

            { Action::ResetMenu, Scope::UI, '2', true,
                "Emergency Reset", "SHIFT + 2", "Centers the window if it gets lost off-screen." },

            { Action::LockMenu, Scope::UI, '3', true,
                "Lock Menu", "SHIFT + 3", "Lock the menu position and disable mouse dragging." },

            { Action::None, Scope::Viewer, 0, false,
                "Camera Speed", "CTRL + WHEEL", "Free camera. While Ctrl is held the camera moves at the custom speed and the wheel changes it." },

            { Action::None, Scope::Viewer, 0, false,
                "Camera Field of View", "WHEEL", "Free camera. Changes the field of view of the current zoom state." },

            { Action::None, Scope::Viewer, 0, false,
                "Select Object", "LEFT CLICK", "Free camera. Selects the object under the crosshair." },

            { Action::None, Scope::Viewer, 0, false,
                "Camera Zoom", "RIGHT CLICK", "Free camera. Turns the zoom on or off." },
        } };
    };
}