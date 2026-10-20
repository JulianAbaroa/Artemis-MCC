export module UI.Hotkey.State;

import UI.Hotkey.Type;
import std;

export namespace UI::Hotkey::State
{
    // Table of the hotkeys the application listens to.
    class HotkeyUIStore
    {
    private:
        using Action = UI::Hotkey::Type::Action;
        using Scope = UI::Hotkey::Type::Scope;
        using Binding = UI::Hotkey::Type::Binding;

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
        static constexpr std::uint32_t k_VkOem5{ 0xDC };
        static constexpr std::uint32_t k_VkF2{ 0x71 };
        static constexpr std::uint32_t k_VkF3{ 0x72 };
        static constexpr std::uint32_t k_VkF4{ 0x73 };
        static constexpr std::uint32_t k_VkF6{ 0x75 };
        static constexpr std::uint32_t k_VkF7{ 0x76 };
        static constexpr std::uint32_t k_VkLeft{ 0x25 };
        static constexpr std::uint32_t k_VkUp{ 0x26 };

        const std::array<Binding, 16> m_Bindings
        { {
            { Action::ToggleLauncher, Scope::UI, k_VkF2, false,
                "Toggle Launcher", "F2", "Show or hide the launcher." },

            { Action::ToggleOverlay, Scope::UI, k_VkF3, false,
                "Toggle Overlay", "F3", "Show or hide the Overlay." },

            { Action::None, Scope::Viewer, k_VkF4, false,
                "Toggle Viewer", "F4", "Show or hide the Viewer." },

            { Action::None, Scope::Viewer, k_VkTab, false,
                "Switch Viewer Mode", "TAB", "Alternates between the free camera and the game camera while the Viewer is active." },

            { Action::None, Scope::Viewer, k_VkOem5, false,
                "Cycle Camera Follow", "|", "Free camera. Cycles the selected object between no follow, first person and orbit. Key left of the 1." },

            { Action::None, Scope::Viewer, k_VkF6, false,
                "Toggle Labels", "F6", "Show or hide the 3D labels over the map." },

            { Action::ToggleMenu, Scope::UI, '1', true,
                "Toggle Menu", "SHIFT + 1", "Show or hide the Artemis Control Panel." },

            { Action::ResetMenu, Scope::UI, '2', true,
                "Emergency Reset", "SHIFT + 2", "Centers the window if it gets lost off-screen." },

            { Action::LockMenu, Scope::UI, '3', true,
                "Lock Menu", "SHIFT + 3", "Lock the menu position and disable mouse dragging." },

            { Action::CaptureDatum, Scope::UI, k_VkF7, false,
                "Capture Datum", "F7", "Copies the datum of the local biped, its linked objects and the pinned objects to the capture file (Capture tab)." },

            { Action::None, Scope::Viewer, k_VkLeft, false,
                "Overlay Mode", "LEFT / RIGHT", "Cycles the overlay panel while the map is active." },

            { Action::None, Scope::Viewer, k_VkUp, false,
                "Overlay Page", "UP / DOWN", "Scrolls the overlay panel while the map is active." },

            { Action::None, Scope::Viewer, 0, false,
                "Camera Speed", "CTRL + WHEEL", "Free camera. While Ctrl is held the camera moves at the custom speed and the wheel changes it." },

            { Action::None, Scope::Viewer, 0, false,
                "Camera Field of View / Orbit Distance", "WHEEL", "Free camera. Changes the field of view of the current zoom state, or the distance while orbiting." },

            { Action::None, Scope::Viewer, 0, false,
                "Select Object", "LEFT CLICK", "Free camera. Selects the object under the crosshair. While following or orbiting it releases the selection instead." },

            { Action::None, Scope::Viewer, 0, false,
                "Camera Zoom", "RIGHT CLICK", "Free camera. Turns the zoom on or off." },
        } };
    };
}