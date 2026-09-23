export module Platform.Input.Type;

import std;

export namespace Platform::Input::Type
{
	// Maps specific game actions to their internal engine Button IDs.
	// These values correspond to the IDs expected by the engine's input polling logic.
	enum class Action
	{
		Unknown = -1,

		// UI and Menu Controls.
		PauseMenu = 0,
		ToggleScoreboard = 30,

		// Theater Mode Controls.
		TogglePanel = 59,
		ToggleUIMode = 58,
		TogglePOVMode = 60,
		NextPlayer = 78,
		PreviousPlayer = 77,
		JumpForward = 80,
		JumpBackward = 79,
		PlayPause = 56,
		FastForward = 33,
		ToggleCameraMode = 72,
		Boost = 57,
		FasterBoost = 69,
		Ascend = 34,
		Descend = 48,

		CameraReset = -100,			// Internal logic (No direct GetButtonState mapping).
		TheaterPanning = -101,		// I didn't get this one.
	};

	// Defines the operational scope for inputs.
	// Ensures the Director only injects commands when the engine is in the correct state.
	enum class Context
	{
		Unknown, Communication, Movement,
		Actions, VehicleControls, UIControls,
		Theater, Forge,
	};

	// Data structure used to request an input injection with specific parameters.
	struct Request
	{
		Context Context{};
		Action Action{};
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