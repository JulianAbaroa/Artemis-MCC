export module Platform.Input.Type;

import Layout.Action.Type;
import std;

namespace
{
    using Layout::Action::Type::k_HoldLanes;
    using Layout::Action::Type::Size::k_MaxActionPlayers;
    using Layout::Action::Type::Size::k_PlayerActionSize;
    using Layout::Action::Type::Size::k_ActionInteractionSize;
}

export namespace Platform::Input::Type
{
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
    // to virtual-key codes (haloreach.dll rva 0x98FDF0).
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

    // Player control tag of the engine globals (A17 sec. 8): the tag address is at the table + k_PlayerControlTagOffset, and the
    // hold threshold (float, seconds) at k_HoldThresholdOffset of the tag data.
    inline constexpr std::uintptr_t k_PlayerControlTagOffset{ 0x418 };
    inline constexpr std::uintptr_t k_HoldThresholdOffset{ 0x48 };

    // Inside the PlayerControlTagLoad match: rel32 of the table variable (+3, next instruction at +7) and rel32 of the call to the
    // tag address resolver (+0x30, next instruction at +0x34).
    inline constexpr std::uintptr_t k_TagLoadTableRelOffset{ 0x03 };
    inline constexpr std::uintptr_t k_TagLoadTableNextOffset{ 0x07 };
    inline constexpr std::uintptr_t k_TagLoadCallRelOffset{ 0x30 };
    inline constexpr std::uintptr_t k_TagLoadCallNextOffset{ 0x34 };

    // Signature of the engine function that turns a tag address into a pointer (haloreach.dll rva 0x15918).
    using ResolveTagAddressFunction = auto(__fastcall*)(const std::uint32_t* address) -> std::uintptr_t;

    // Command of one player for one tick, as the simulation applies it.
    // note: Opaque except the offsets of Layout.Action.Type. The engine builds it once per frame from the player control of each local player.
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

    // Player control globals: pointer inside the thread context of the engine, one block per local player (A9 sec. 11.2 and 12.1).
    // note: The view of each split-screen player follows the facing of its block, not the action.
    inline constexpr std::uintptr_t k_ControlGlobalsSlot{ 0xD8 };
    inline constexpr std::uintptr_t k_ControlBlockStride{ 0x180 };

    // Desired yaw and pitch of a control block (2 floats, radians), as the frame input left them.
    inline constexpr std::uintptr_t k_ControlFacingOffset{ 0x94 };

    // Fields the shadow check compares (D122): what Artemis would send as the client of a local player against what the engine built.
    enum class ShadowField : std::uint8_t
    {
        // Interaction records (+0x54), byte for byte.
        Interaction = 0,
        // Interaction mismatches where the engine matches what Artemis built one tick before (the local pipeline is one tick late).
        InteractionLate,
        // Held bit of every hold lane that is down (+0x40), within one tick.
        HeldLane,
        // Weapon slot asked for (+0x46) when it differs from the one in the hands.
        Weapon,
        // Grenade type asked for (+0x48) when it differs from the current one.
        Grenade,
        // Zoom level asked for (+0x4A) when it differs from the current one and is not "no zoom".
        Zoom,

        Count
    };

    inline constexpr std::size_t k_ShadowFieldCount{ static_cast<std::size_t>(ShadowField::Count) };

    // Names of the fields, in the order of ShadowField (log and UI).
    inline constexpr std::array<const char*, k_ShadowFieldCount> k_ShadowFieldNames
    {
        "Interaction",
        "Interaction (1 tick late)",
        "Held lane",
        "Weapon",
        "Grenade",
        "Zoom",
    };

    // Mismatches of each field the shadow check logs. The rest are only counted.
    inline constexpr std::uint64_t k_ShadowLogLimit{ 20 };

    // What the shadow check remembers of one local player between ticks (D122).
    struct ShadowPlayer
    {
        // Ticks each hold lane has been down, and whether it was seen up since the check started (else its count is unknown).
        std::array<std::uint32_t, k_HoldLanes.size()> LaneTicks{};
        std::array<bool, k_HoldLanes.size()> IsLaneSeenUp{};

        // Interaction Artemis built on the previous tick, and whether there is one.
        std::array<std::uint8_t, k_ActionInteractionSize> PreviousInteraction{};
        bool HasPreviousInteraction{ false };

        // Weapon slot and grenade type the engine asked for on the previous tick (the empty action values at first).
        std::uint8_t WeaponSlot{ 0xFF };
        std::uint16_t GrenadeType{ 0xFFFF };
    };

    // Comparisons of one field since the shadow check was cleared.
    struct ShadowCount
    {
        std::uint64_t Checked{ 0 };
        std::uint64_t Mismatched{ 0 };
    };

    // What the action of one player did since the watch was cleared, so presses of a single tick are not missed (A9 M4a).
    struct ActionWatch
    {
        // Every button bit held at some tick.
        std::uint64_t SeenButtons{ 0 };

        // First action after the clear. The bytes are compared with it.
        bool HasReference{ false };
        PlayerAction Reference{};

        // Bytes that differed from the reference at some tick, and the last value they had then.
        std::array<bool, k_PlayerActionSize> IsChanged{};
        PlayerAction LastChanged{};
    };

    // Signature of the engine function that applies the player actions of one tick (haloreach.dll rva 0x47050).
    // note: ExecuteGameFrame calls it once per tick with the simulation update of that tick.
    using ApplyPlayerActionsFunction = auto(__fastcall*)(void* update) -> void;

    // Signature of the engine function that clears an action to the empty action of the engine (haloreach.dll rva 0x46a34).
    // note: Empty means no unit, no weapon, grenade or zoom request, no interaction and no aim assist target (A17 sec. 4).
    using ClearPlayerActionFunction = auto(__fastcall*)(void* action) -> void;
}