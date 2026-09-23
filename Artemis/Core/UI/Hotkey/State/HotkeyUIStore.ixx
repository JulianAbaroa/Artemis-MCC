export module UI.Hotkey.State;

import UI.Hotkey.Type;
import std;

export namespace UI::Hotkey::State
{
	class HotkeyUIStore
	{
	private:
		using Action = UI::Hotkey::Type::Action;
		using Scope = UI::Hotkey::Type::Scope;
		using Binding = UI::Hotkey::Type::Binding;

		static constexpr std::uint32_t k_VK_F2 = 0x71;
		static constexpr std::uint32_t k_VK_F3 = 0x72;
		static constexpr std::uint32_t k_VK_F4 = 0x73;
		static constexpr std::uint32_t k_VK_F5 = 0x74;
		static constexpr std::uint32_t k_VK_Left = 0x25;
		static constexpr std::uint32_t k_VK_Up = 0x26;

	public:
		HotkeyUIStore() = default;
		~HotkeyUIStore() = default;

		HotkeyUIStore(const HotkeyUIStore&) = delete;
		HotkeyUIStore& operator=(const HotkeyUIStore&) = delete;

		auto GetBindings() const -> std::span<const Binding>;

		auto Find(std::uint32_t virtualKey, bool isShiftDown) const -> const Binding*;

	private:
		const std::array<Binding, 9> m_Bindings
		{ {
			{ Action::ToggleLauncher, Scope::UI, k_VK_F2, false,
				"Toggle Launcher", "F2", "Show or hide the launcher." },

			{ Action::ToggleOverlay, Scope::UI, k_VK_F3, false,
				"Toggle Overlay", "F3", "Show or hide the Overlay." },

			{ Action::None, Scope::Viewer, k_VK_F4, false,
				"Toggle Map", "F4", "Show or hide the Map." },

			{ Action::None, Scope::Viewer, k_VK_F5, false,
				"Toggle Camera Follow", "F5", "Makes the map camera follow the selection." },

			{ Action::ToggleMenu, Scope::UI, '1', true,
				"Toggle Menu", "SHIFT + 1", "Show or hide the Artemis Control Panel." },

			{ Action::ResetMenu, Scope::UI, '2', true,
				"Emergency Reset", "SHIFT + 2", "Centers the window if it gets lost off-screen." },

			{ Action::LockMenu, Scope::UI, '3', true,
				"Lock Menu", "SHIFT + 3", "Lock the menu position and disable mouse dragging." },

			{ Action::None, Scope::Viewer, k_VK_Left, false,
				"Overlay Mode", "LEFT / RIGHT", "Cycles the overlay panel while the map is active." },

			{ Action::None, Scope::Viewer, k_VK_Up, false,
				"Overlay Page", "UP / DOWN", "Scrolls the overlay panel while the map is active." },
		} };
	};
}