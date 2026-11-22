export module Engine.Control.Input.Type;

import Control.Client.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Blam::Layout::Action::Type::k_HoldLanes;
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
    using Blam::Layout::Action::Type::Size::k_PlayerActionSize;
    using Blam::Layout::Action::Type::Size::k_ActionInteractionSize;
}

export namespace Engine::Control::Input::Type
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

    // Where the engine keeps the player control tag: the variable that points to the globals table that holds its address, and the
    // function that resolves a tag address. Both null when the match was not found.
    struct ControlTagLocator
    {
        const std::uintptr_t* Table{ nullptr };
        ResolveTagAddressFunction Resolve{ nullptr };

        // return: Whether both were found.
        constexpr auto IsValid() const -> bool { return Table != nullptr && Resolve != nullptr; }
    };

    // Command of one player for one tick, as the simulation applies it.
    // note: Opaque except the offsets of Blam.Layout.Action.Type. The engine builds it once per frame from the player control of each local player.
    struct PlayerAction
    {
        std::array<std::uint8_t, k_PlayerActionSize> Data{};
    };

    static_assert(sizeof(PlayerAction) == k_PlayerActionSize);

    // return: The value of type T at the offset of the action.
    template<typename T>
    auto ReadAction(const PlayerAction& action, std::uintptr_t offset) -> T
    {
        T value{};
        std::memcpy(&value, action.Data.data() + offset, sizeof(T));
        return value;
    }

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

        // Client oracle (VW.9c.1, D356): calls of the engine client compared with the pure client, by output field. Checked counts the
        // calls compared, Mismatched the calls where the field differs. The bits and values go to the log and the oracle file.
        ClientMove,
        ClientLook,
        ClientTriggers,
        ClientButtons,
        ClientEvents,
        ClientLanes,
        ClientState,

        // Calls that read a fact of the unit the port does not know (ClientOutput::UnknownRead). They are NOT counted in the fields above:
        // Checked counts them, Mismatched the ones where something also differs. A mismatch here may come from the missing fact.
        ClientPartial,

        // Calls not compared: the table is not the keyboard and mouse one, the pure client reported a branch it does not run, the output
        // was not clean, or the context was not readable. Checked counts them, Mismatched stays 0.
        ClientNotComparable,

        // Calls compared by context of the unit, and the calls of each that differed: on foot, in a seat, with zoom and without a unit.
        ClientOnFoot,
        ClientSeated,
        ClientZoomed,
        ClientNoUnit,

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
        "Client move",
        "Client look",
        "Client triggers",
        "Client buttons",
        "Client events",
        "Client lanes",
        "Client state",
        "Client partial",
        "Client not comparable",
        "Client on foot",
        "Client seated",
        "Client zoomed",
        "Client no unit",
    };

    // One count of the shadow check: the field and whether the call matched on it.
    struct ClientCount
    {
        ShadowField Field{ ShadowField::ClientMove };
        bool IsMatch{ true };
    };

    // The counts one compared call adds. At most 10: the 7 output fields, the context of the unit and the zoom.
    struct ClientCounts
    {
        std::array<ClientCount, 10> Items{};
        std::size_t Size{ 0 };

        constexpr auto Add(ShadowField field, bool isMatch) -> void
        {
            Items[Size] = ClientCount{ field, isMatch };
            ++Size;
        }
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

    // Player control of a local player (haloreach.dll rva 0x5df80): reads the event table of the input user and fills the output (0x60
    // bytes the caller clears) and the client block. The client oracle hooks it (VW.9c.1, D356).
    // note: The two floats are the seconds of the tick (xmm2) and a value the caller passes along (xmm3). The table pointer is written
    // through the fifth argument.
    using PlayerControlFunction = auto(__fastcall*)(std::int32_t inputUser, std::uint32_t localIndex, float deltaTime, float unused,
        void* tableOut, float* output) -> void;

    inline constexpr std::uint32_t k_PlayerControlRva{ 0x5DF80 };

    // How far from the function start the PlayerControlTagLoad match may begin (it begins at +0x8C).
    inline constexpr std::uint32_t k_PlayerControlAnchorWindow{ 0x90 };

    // Event table of the input users (g_InputEventTable_Player0, A17 sec. 9): rva, bytes per input user, input users the client handles.
    // note: The oracle compares only input user 0: the client fields of a block reach +0x1E2 past the 0x180 stride of 0x5df80, so the
    // block of users 1-3 overlaps the next one and user 3 would read past the last block (unconfirmed in game, VW.9c.1 report).
    inline constexpr std::uintptr_t k_InputEventTableRva{ 0x287FF94 };
    inline constexpr std::size_t k_InputTableSize{ 0x700 };
    inline constexpr std::size_t k_ClientInputUsers{ 1 };

    // Event entries of the table in the keyboard and mouse mode (+0x224 + 0x10 * event) and their fields.
    inline constexpr std::size_t k_InputEventsOffset{ 0x224 };
    inline constexpr std::size_t k_InputEventSize{ 0x10 };
    inline constexpr std::size_t k_EventHeldOffset{ 0x0 };
    inline constexpr std::size_t k_EventDownOffset{ 0x2 };
    inline constexpr std::size_t k_EventFlagsOffset{ 0x3 };
    inline constexpr std::size_t k_EventValueOffset{ 0x4 };
    inline constexpr std::size_t k_EventLinkOffset{ 0x8 };
    inline constexpr std::size_t k_EventLinkAnyOffset{ 0xC };
    inline constexpr std::uint32_t k_EventLinkAny{ 0x7F };

    // Tail of the table: move axes, look deltas, the tail value and the mouse flag.
    inline constexpr std::size_t k_InputMoveForwardOffset{ 0x6BC };
    inline constexpr std::size_t k_InputMoveSideOffset{ 0x6C0 };
    inline constexpr std::size_t k_InputYawDeltaOffset{ 0x6C4 };
    inline constexpr std::size_t k_InputPitchDeltaOffset{ 0x6C8 };
    inline constexpr std::size_t k_InputPitchDeltaAltOffset{ 0x6CC };
    inline constexpr std::size_t k_InputTailOffset{ 0x6D0 };
    inline constexpr std::size_t k_InputMouseFlagOffset{ 0x6F8 };

    // Control block of an input user: the client block starts at +0x80 of it (the pointer 0x5df80 uses), and this many bytes of it are
    // kept for the oracle: the highest field the port reads (the grenade latch, +0x162) rounded up to 4.
    // note: The fields past +0x100 of the client block land beyond k_ControlBlockStride (0x80 + 0x168 > 0x180). The stride of the task file
    // and the offsets of the decompile do not agree: one of them is wrong. Only user 0 is certain (it does not depend on the stride).
    inline constexpr std::uintptr_t k_ClientBlockOffset{ 0x80 };
    inline constexpr std::size_t k_ClientBlockWindow{ 0x168 };

    // Output of 0x5df80 (0x60 bytes, cleared by the caller). The oracle compares the first 0x34 bytes: the interaction records come
    // after.
    inline constexpr std::size_t k_ClientOutputSize{ 0x60 };
    inline constexpr std::size_t k_ClientOutputCompared{ 0x34 };

    // Fields of that output: move axes, triggers, look delta, buttons, event bits, hold lanes and the tail value.
    inline constexpr std::size_t k_OutputMoveOffset{ 0x00 };
    inline constexpr std::size_t k_OutputFireTriggerOffset{ 0x08 };
    inline constexpr std::size_t k_OutputZoomTriggerOffset{ 0x0C };
    inline constexpr std::size_t k_OutputTrigger4Offset{ 0x10 };
    inline constexpr std::size_t k_OutputLookOffset{ 0x14 };
    inline constexpr std::size_t k_OutputButtonsOffset{ 0x20 };
    inline constexpr std::size_t k_OutputEventsOffset{ 0x28 };
    inline constexpr std::size_t k_OutputLanesOffset{ 0x2C };
    inline constexpr std::size_t k_OutputTailOffset{ 0x30 };

    // Fields of the client block (offsets from its start) the oracle reads and compares.
    inline constexpr std::size_t k_ClientSelectionIndexOffset{ 0x48 };
    inline constexpr std::size_t k_ClientSelectionKindOffset{ 0x4C };
    inline constexpr std::size_t k_ClientYawOffset{ 0x14 };
    inline constexpr std::size_t k_ClientZoomLevelOffset{ 0x46 };
    inline constexpr std::size_t k_ClientLookRateActiveOffset{ 0x9C };
    inline constexpr std::size_t k_ClientLookRateYawOffset{ 0xA0 };
    inline constexpr std::size_t k_ClientLookRemainderYawOffset{ 0xB0 };
    inline constexpr std::size_t k_ClientFrameHandleOffset{ 0x114 };
    inline constexpr std::size_t k_ClientJumpTimerOffset{ 0x138 };
    inline constexpr std::size_t k_ClientAimAssistOffset{ 0x140 };
    inline constexpr std::size_t k_ClientCrouchOffset{ 0x14C };
    inline constexpr std::size_t k_ClientCrouchHeldOffset{ 0x150 };
    inline constexpr std::size_t k_ClientSwitchWeaponOffset{ 0x15C };
    inline constexpr std::size_t k_ClientGrenadeOffset{ 0x15F };
    static_assert(k_ClientGrenadeOffset + 4 <= k_ClientBlockWindow, "The window must cover the highest client field the port reads");

    // Settings of the local users the client reads (A17 sec. 8 and 9.1): rva of the array of settings blocks, bytes of each, the block used
    // when bit 14 of the first word says "default", and the fields of a block: aim scale with zoom and in a seat, and the jump flag.
    inline constexpr std::uintptr_t k_ControlSettingsRva{ 0x2A03C58 };
    inline constexpr std::uintptr_t k_ControlSettingsStride{ 0xAB8 };
    inline constexpr std::uintptr_t k_ControlSettingsDefaultRva{ 0x2A06910 };
    inline constexpr std::uint32_t k_ControlSettingsDefaultBit{ 14 };
    inline constexpr std::size_t k_SettingsAimZoomedOffset{ 0x60 };
    inline constexpr std::size_t k_SettingsAimSeatedOffset{ 0x64 };
    inline constexpr std::size_t k_SettingsJumpFlagOffset{ 0x69 };

    // Globals the client reads: the crouch toggle byte and the byte that makes melee count for the use lane, by rva.
    inline constexpr std::uintptr_t k_CrouchToggleRva{ 0x287FF7A };
    inline constexpr std::uintptr_t k_MeleeHoldsUseRva{ 0x4E2FCB1 };

    // Dual wield restrictions byte of the user (1 no throw, 2 no zoom, 4 no zoom in), by rva (VW.9c.2): with the keyboard and mouse
    // one global, with a pad one byte in the settings of the local user. The mode of each local user (0 pad, 1 keyboard and mouse) is
    // an int32 array of four.
    inline constexpr std::uintptr_t k_InputModeRva{ 0x2DFBE74 };
    inline constexpr std::uint32_t k_InputModeCount{ 4 };
    inline constexpr std::uintptr_t k_DualWieldKeyboardRva{ 0x287FF90 };
    inline constexpr std::uintptr_t k_DualWieldPadRva{ 0x287F668 };
    inline constexpr std::uintptr_t k_DualWieldPadStride{ 0x7C };

    // Fields of the control tag (the hold threshold is k_HoldThresholdOffset): zoom cancel threshold and jump timer seconds.
    inline constexpr std::size_t k_ControlTagZoomCancelOffset{ 0x4C };
    inline constexpr std::size_t k_ControlTagJumpTimerOffset{ 0x88 };

    // Clock of the thread context: pointer slot, ticks per second (int16) and seconds of a tick (float).
    inline constexpr std::uintptr_t k_ClockSlot{ 0xA0 };
    inline constexpr std::size_t k_ClockTicksPerSecondOffset{ 0x06 };
    inline constexpr std::size_t k_ClockTickSecondsOffset{ 0x08 };

    // Mismatching calls of the client oracle the log shows, and the file of AppData that gets one line per mismatch.
    inline constexpr std::uint64_t k_ClientLogLimit{ 30 };

    // Lines the oracle file takes at most per session, and lines the buffer between the game thread and the Main thread holds (a full
    // buffer drops the line and counts it).
    inline constexpr std::uint64_t k_ClientMaxFileLines{ 5000 };
    inline constexpr std::size_t k_ClientBufferLines{ 64 };

    // Lines kept per kind of difference (the line without the dt, the table and the block): one systematic difference (a unit fact not
    // resolved yet) would otherwise fill the file in seconds and hide the rest.
    inline constexpr std::uint32_t k_ClientLinesPerKind{ 20 };
    inline constexpr const char* k_ClientOracleFile{ "client_oracle.txt" };

    // Fields of ClientState the oracle compares, one bit each in ClientDifferences::StateFields.
    enum class ClientStateField : std::uint8_t
    {
        Selection = 0,
        LookRates,
        Zoom,
        AimAssist,
        Frame,
        Jump,
        Crouch,
        SwitchWeapon,
        Grenade,

        Count
    };

    inline constexpr std::array<const char*, static_cast<std::size_t>(ClientStateField::Count)> k_ClientStateFieldNames
    {
        "selection", "look rates", "zoom level", "aim assist", "frame of reference", "jump timer", "crouch", "switch weapon", "grenade",
    };

    // What differs between the engine client and the pure client in one call.
    struct ClientDifferences
    {
        bool IsMove{ false };
        bool IsLook{ false };
        bool IsTriggers{ false };

        // Bits that differ in the buttons, the events and the lanes.
        std::uint64_t Buttons{ 0 };
        std::uint32_t Events{ 0 };
        std::uint32_t Lanes{ 0 };

        // The tail value (+0x30) differs.
        bool IsTail{ false };

        // One bit per ClientStateField.
        std::uint32_t StateFields{ 0 };

        // return: Whether nothing differs.
        constexpr auto IsEmpty() const -> bool
        {
            return !IsMove && !IsLook && !IsTriggers && !IsTail && Buttons == 0 && Events == 0 && Lanes == 0 && StateFields == 0;
        }
    };

    // What the Viewer panel shows of the possession of a bot (VW.9c.2 B, D352): a counter, nothing is compared (the oracle compares the
    // local player). Written by the game thread, read by the UI.
    struct PossessStatus
    {
        // Handle of the bot that is possessed, 0xFFFFFFFF when none.
        std::uint32_t Player{ 0xFFFFFFFF };

        // Ticks the bot was possessed, and the ones whose action came from the client of the user (the others had a neutral action: the
        // unit of the bot or the table of the user was missing).
        std::uint64_t Ticks{ 0 };
        std::uint64_t Delivered{ 0 };

        // The client had a branch it does not port in the last delivered tick (see Unsupported).
        std::uint64_t Unsupported{ 0 };

        // The unit of the possessed bot was missing in some tick (one WARNING in the log per possession).
        bool IsUnitMissing{ false };
    };

    // The input user whose table and settings drive the possessed bot, and the local player the possession leaves still (VW.9c.2 B, D353, D359).
    inline constexpr std::int32_t k_PossessInputUser{ 0 };

    // A possess request older than this is no request. The Viewer asks at every Present, so an older one means nobody asks any more (the Viewer was
    // turned off, a stalled Present, a map load) and the bot must be given back.
    inline constexpr std::chrono::milliseconds k_PossessRequestMaxAge{ 100 };

    // Ticks a possessed bot may go without a new frame of the client before it gets a neutral action and forgets its held state (D360). Two ticks at
    // x1.0 are about 66 ms: a frame that did not run (table not in the mouse state, no settings, input user 0 unmapped) must not repeat its last output.
    inline constexpr std::uint32_t k_PossessMaxStaleTicks{ 2 };

    // What the client keeps for the possessed bot between frames and ticks (VW.9c.2 B, D350, D360): its own client state, never the control block of
    // a local player. The client runs once per FRAME (the 0x5df80 hook) and the ticks only copy what it accumulated, like 0x628b0 copies the block.
    struct PossessedBot
    {
        // Player index of the bot and its handle with the salt: another bot in the same slot is another possession.
        std::size_t Player{ 0 };
        std::uint32_t Handle{ 0xFFFFFFFF };

        // Unit the aim was started for. A new unit (respawn) starts the aim again from the unit.
        std::uint32_t Unit{ 0xFFFFFFFF };

        // Client state after the last frame. Its yaw and pitch are the aim, accumulated over every frame.
        ::Control::Client::Type::ClientState State{};

        // Whether a frame ran since the possession (or the unit) began.
        bool HasFrame{ false };

        // Ticks since the last frame ran. The frame sets it to 0, every tick adds one (TickFrameAge).
        std::uint32_t TicksSinceFrame{ 0 };

        // Output of the last frame (the held state: move, buttons, lanes, triggers, held events).
        ::Control::Client::Type::ClientOutput Last{};

        // What the frames since the last tick asked for: the buttons, lanes and pulse events OR-ed, the triggers at their largest. A tick delivers it
        // once and clears it, so a press is neither lost between ticks nor delivered twice.
        ::Control::Client::Type::ClientOutput Pending{};
    };

    // Signature of the engine function that applies the player actions of one tick (haloreach.dll rva 0x47050).
    // note: ExecuteGameFrame calls it once per tick with the simulation update of that tick.
    using ApplyPlayerActionsFunction = auto(__fastcall*)(void* update) -> void;

    // Signature of the engine function that clears an action to the empty action of the engine (haloreach.dll rva 0x46a34).
    // note: Empty means no unit, no weapon, grenade or zoom request, no interaction and no aim assist target (A17 sec. 4).
    using ClearPlayerActionFunction = auto(__fastcall*)(void* action) -> void;

    // Signature of the engine function that copies an action into the input of a view (haloreach.dll rva 0x62fcc).
    // note: Moves the aim, zoom, weapon and buttons of the input of the view, which the first person camera reads (A21 sec. 9.2).
    using CopyActionToViewFunction = auto(__fastcall*)(std::int32_t view, const void* action) -> void;
}