export module UI.Hotkey.Type;

import std;

export namespace UI::Hotkey::Type
{
	enum class Action : std::uint8_t
	{
		None = 0,

		ToggleLauncher,
		ToggleOverlay,
		ToggleMenu,
		ResetMenu,
		LockMenu,
	};

	enum class Scope : std::uint8_t
	{
		UI,
		Viewer,
	};

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