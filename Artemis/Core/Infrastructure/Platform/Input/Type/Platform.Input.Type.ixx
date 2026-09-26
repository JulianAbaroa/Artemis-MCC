export module Platform.Input.Type;

import std;

export namespace Platform::Input::Type
{
	// Maps specific game actions to their internal engine Button IDs.
	// These values correspond to the IDs expected by the engine's input polling logic.
	enum class Action
	{
		Unknown = -1,

		W = 32,					// Move forward
		A = 45,					// Move left
		S = 46,					// Move backward
		D = 47,					// Move right
		Space = 72,				// Jump
		LeftControl = 69,		// Crouch
		Z = 58,					// Zoom in
		X = 59,					// Zoom out
		Q = 31,					// Melee
		R = 34,					// Reload
		F = 48,					// Throw grenade
		E = 33,					// Interact
		One = 17,				// Change weapon
		Two = 18,				// Change grenade
		Shift = 57,				// Use armor ability

		C = 60,					// Shoot
	};

	struct WindowMessage
	{
		void* Window{ nullptr };
		std::uint32_t Message{ 0 };
		std::uintptr_t WParam{ 0 };
		std::intptr_t LParam{ 0 };
		std::intptr_t Result{ 0 };
	};

	struct RawMouse
	{
		std::int32_t DeltaX{ 0 };
		std::int32_t DeltaY{ 0 };

		std::uint16_t ButtonFlags{ 0 };
		std::uint16_t ButtonData{ 0 };
	};
}