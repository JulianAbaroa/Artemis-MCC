module;

#include <windows.h>

export module Platform.Input.Type;

import std;

export namespace Platform::Input::Type
{
    // Signature of the engine function that polls a button.
    using GetButtonStateFunction = auto(__fastcall*)(short buttonID) -> char;

    // Signature of GetRawInputData from user32.
    using GetRawInputDataFunction = auto(WINAPI*)(HRAWINPUT, UINT, LPVOID, PUINT, UINT) -> UINT;

    // Game actions mapped to the internal button IDs the engine polls.
    enum class InputAction
    {
        Unknown = -1,

        W = 32,                 // Move forward
        A = 45,                 // Move left
        S = 46,                 // Move backward
        D = 47,                 // Move right
        Space = 72,             // Jump
        LeftControl = 69,       // Crouch
        Z = 58,                 // Zoom in
        X = 59,                 // Zoom out
        Q = 31,                 // Melee
        R = 34,                 // Reload
        F = 48,                 // Throw grenade
        E = 33,                 // Interact
        One = 17,               // Change weapon
        Two = 18,               // Change grenade
        Shift = 57,             // Use armor ability

        C = 60,                 // Shoot
    };

    // Source of the input of a local player.
    enum class LocalInputSource : std::uint8_t
    {
        // MCC reads the real devices (keyboard and mouse only for player 0).
        Device = 0,
        // Artemis gives the state (VirtualInputStore).
        Virtual,
    };

    // Mouse buttons, as bits of MouseState::Buttons.
    enum class MouseButton : std::uint8_t
    {
        Left = 0,
        Right,
        Middle,
        Button4,
        Button5,
    };

    // Gamepad buttons, as bits of GamepadState::Buttons.
    enum class GamepadButton : std::uint8_t
    {
        DpadUp = 0,
        DpadDown,
        DpadLeft,
        DpadRight,
        Start,
        Back,
        LeftThumb,
        RightThumb,
        LeftShoulder,
        RightShoulder,
        A,
        B,
        X,
        Y,
    };

    // Mouse part of the input state.
    struct MouseState
    {
        // Relative movement since the last frame.
        float DeltaX{ 0.0f };
        float DeltaY{ 0.0f };

        // Absolute position.
        float PositionX{ 0.0f };
        float PositionY{ 0.0f };

        float DeltaWheel{ 0.0f };
        float WheelScale{ 0.0f };

        // One bit per MouseButton.
        std::uint32_t Buttons{ 0 };
    };

    static_assert(sizeof(MouseState) == 0x1C);

    // Gamepad part of the input state (XInput ranges).
    struct GamepadState
    {
        // One bit per GamepadButton.
        std::uint32_t Buttons{ 0 };

        std::uint8_t LeftTrigger{ 0 };
        std::uint8_t RightTrigger{ 0 };

        std::int16_t LeftX{ 0 };
        std::int16_t LeftY{ 0 };
        std::int16_t RightX{ 0 };
        std::int16_t RightY{ 0 };
    };

    static_assert(sizeof(GamepadState) == 0x10);

    // Input of one local player as MCC gives it to the engine once per frame (libmcc s_input_state).
    // note: Keys is indexed by Windows virtual-key code. The engine reads it through a table that maps its own button ids
    // (InputAction) to virtual-key codes (haloreach.dll rva 0x98FDF0).
    struct InputState
    {
        // 1 = keyboard and mouse, 0 = gamepad.
        std::int32_t IsKeyboardMouse{ 0 };

        std::array<bool, 256> Keys{};
        MouseState Mouse{};
        GamepadState Gamepad{};
    };

    static_assert(sizeof(InputState) == 0x130);

    // Signature of the MCC function that fills the input of a local player.
    // return: True if the state is valid for this frame.
    using InputUpdateFunction = auto(__fastcall*)(void* gameManager, std::int32_t player, InputState* state) -> bool;

    // Players a simulation update holds actions for, by player table index (A9_SplitScreen.txt sec. 12).
    inline constexpr std::size_t k_MaxActionPlayers{ 16 };

    // Bytes of the action of one player.
    inline constexpr std::size_t k_PlayerActionSize{ 0xA8 };

    // Mask of the players with an action (uint32, one bit per player table index), inside the simulation update.
    inline constexpr std::uintptr_t k_UpdateActionMaskOffset{ 0x08 };

    // First action inside the simulation update. The action of a player is at this offset + k_PlayerActionSize * its table index.
    inline constexpr std::uintptr_t k_UpdateActionsOffset{ 0x10 };

    // Buttons of the action (uint64). The meaning of each bit is being measured (A9 M4a).
    inline constexpr std::uintptr_t k_ActionButtonsOffset{ 0x08 };

    // Desired yaw, pitch and roll of the action (3 floats, absolute).
    inline constexpr std::uintptr_t k_ActionFacingOffset{ 0x10 };

    // Throttle of the action (2 floats).
    inline constexpr std::uintptr_t k_ActionThrottleOffset{ 0x1C };

    // Command of one player for one tick, as the simulation applies it.
    // note: Opaque except the offsets above. The engine builds it once per frame from the player control of each local player.
    struct PlayerAction
    {
        std::array<std::uint8_t, k_PlayerActionSize> Data{};
    };

    static_assert(sizeof(PlayerAction) == k_PlayerActionSize);

    // Actions of every player in one tick.
    struct PlayerActions
    {
        // Ticks seen by the hook since it was installed.
        std::uint64_t Sequence{ 0 };

        // One bit per player table index with an action this tick.
        std::uint32_t Mask{ 0 };

        std::array<PlayerAction, k_MaxActionPlayers> Actions{};
    };

    // Signature of the engine function that applies the player actions of one tick (haloreach.dll rva 0x47050).
    // note: ExecuteGameFrame calls it once per tick with the simulation update of that tick.
    using ApplyPlayerActionsFunction = auto(__fastcall*)(void* update) -> void;

    // Window message captured by the WndProc hook.
    struct WindowMessage
    {
        void* Window{ nullptr };
        std::uint32_t Message{ 0 };
        std::uintptr_t WParam{ 0 };
        std::intptr_t LParam{ 0 };

        // Value returned to the window when a handler consumes the message.
        std::intptr_t Result{ 0 };
    };

    // Raw mouse movement and buttons captured by the GetRawInputData hook.
    struct RawMouse
    {
        std::int32_t DeltaX{ 0 };
        std::int32_t DeltaY{ 0 };

        std::uint16_t ButtonFlags{ 0 };
        std::uint16_t ButtonData{ 0 };
    };

    // Handler for window messages. Returns true to consume the message.
    using WindowMessageHandler = std::function<bool(WindowMessage&)>;

    // Handler for raw mouse movement. Returns true to swallow the movement.
    using RawMouseHandler = std::function<bool(RawMouse&)>;
}