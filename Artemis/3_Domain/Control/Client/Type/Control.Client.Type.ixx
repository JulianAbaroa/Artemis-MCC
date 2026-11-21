export module Control.Client.Type;

import std;

export namespace Control::Client::Type
{
    // Events of the input event table the client reads (A17 sec. 9). The value is the index of the event in the table.
    // note: Some indices come from calls whose argument the decompiler dropped (D356). They are named here once, so a difference the
    // oracle finds is fixed in one place.
    enum class InputEvent : std::uint8_t
    {
        Jump = 0x00,
        SwitchGrenade = 0x01,
        SwitchWeapon = 0x02,
        Use = 0x03,
        Melee = 0x04,
        Ability = 0x05,
        Throw = 0x06,
        Fire = 0x07,
        Crouch = 0x08,
        Zoom = 0x09,
        ZoomValue = 0x0A,
        SeatCrouch = 0x0C,
        SeatJump = 0x0D,
        SeatCrouchAlt = 0x0E,
        SeatJumpAlt = 0x0F,
        LaneMelee = 0x10,
        Button11 = 0x11,
        Button16 = 0x16,
        Button17 = 0x17,
        Button18 = 0x18,
        Chord19 = 0x19,
        Chord1A = 0x1A,
        Reload = 0x1E,
        ThrowGrenade = 0x1F,
        ZoomAlt = 0x28,
        Pause = 0x35,
        LaneSwitchAlt = 0x36,
        LaneHold = 0x37,
        Button38 = 0x38,
        ZoomIn = 0x3C,
        ZoomOut = 0x3D,
        LaneButton = 0x3E,
        Button46 = 0x46,
        Button47 = 0x47,
        Button48 = 0x48,
    };

    // Events of the table of one input user (0x49).
    inline constexpr std::size_t k_EventCount{ 0x49 };

    // Input mode of the keyboard and the mouse (g_InputProcessing_Mode of the input user). The client only runs in it.
    inline constexpr std::uint8_t k_KeyboardMouseMode{ 1 };

    // Pitch the engine never goes past, radians.
    inline constexpr float k_PitchLimit{ 1.4922565f };

    // Zoom level of the client block when the unit is not zoomed.
    inline constexpr std::int16_t k_NoZoom{ -1 };

    // One event of the table as the client reads it, already resolved to "valid": an entry that is blocked or whose link does not match
    // reads as all zeros (0x5d31c, 0x5d358, 0x5d374).
    struct EventSample
    {
        bool IsDown{ false };

        // Milliseconds held.
        std::uint16_t HeldMs{ 0 };

        // Analog value 0 to 1.
        float Value{ 0.0f };
    };

    // What the client reads from the table of an input user in one call (A17 sec. 9).
    struct InputFrame
    {
        // g_InputProcessing_Mode of the input user, and the mouse flag of the table (+0x6F8, 1 with keyboard and mouse).
        std::uint8_t Mode{ k_KeyboardMouseMode };
        bool IsMouse{ true };

        std::array<EventSample, k_EventCount> Events{};

        // Raw value of the first two slots of the table (+0x04 and +0x14), not the mapped events.
        float RawValue0{ 0.0f };
        float RawValue1{ 0.0f };

        // Tail of the table (+0x6BC to +0x6D0): move axes in [-1, 1], raw mouse deltas (yaw, pitch and its alternate copy) and the value
        // the client copies into the output unread.
        float MoveForward{ 0.0f };
        float MoveSide{ 0.0f };
        float YawDelta{ 0.0f };
        float PitchDelta{ 0.0f };
        float PitchDeltaAlt{ 0.0f };
        float Tail6D0{ 0.0f };
    };

    // Fields of the client block that the client reads and writes between calls. Offsets are from the start of the client block of the
    // input user (the 0x5df80 pointer, control block + 0x80). The control block offset of A17 sec. 7 is this + 0x80.
    struct ClientState
    {
        // +0x48 and +0x4C (selection of the unit by stick): the first is reset to 0xFFFFFFFF with a unit.
        std::uint32_t SelectionIndex{ 0xFFFFFFFF };
        std::uint32_t SelectionKind{ 0xFFFFFFFF };

        // +0x9C accessibility flag of the player, +0xA0 and +0xA4 its clamped look rates, +0xB0 and +0xB4 their remainders.
        std::uint8_t LookRateActive{ 0 };
        float LookRateYaw{ 0.0f };
        float LookRatePitch{ 0.0f };
        float LookRemainderYaw{ 0.0f };
        float LookRemainderPitch{ 0.0f };

        // +0x14 and +0x18 absolute yaw and pitch, radians (control block +0x94 and +0x98). 0x5df80 does not move them (0x6375c does):
        // IntegrateAim does.
        float Yaw{ 0.0f };
        float Pitch{ 0.0f };

        // +0xC6 zoom level of the client (-1 = no zoom). 0x5df80 reads it for the aim factor and resets it for a carried object.
        std::int16_t ZoomLevel{ -1 };

        // +0x140 aim assist is active (always 0 with the mouse).
        std::uint8_t AimAssistActive{ 0 };

        // +0x114 handle of the frame of reference of the unit the last call saw.
        std::uint32_t FrameHandle{ 0xFFFFFFFF };

        // +0x138 and +0x13C timer and phase of the jump press.
        float JumpTimer{ 0.0f };
        std::int32_t JumpPhase{ 0 };

        // +0x14C crouch toggle (the crouch bit of the buttons) and +0x150 seconds the crouch has been held.
        bool IsCrouchToggled{ false };
        float CrouchHeldSeconds{ 0.0f };

        // +0x15C switch weapon was down, +0x15D its latch.
        bool IsSwitchWeaponDown{ false };
        bool IsSwitchWeaponLatched{ false };

        // +0x15F switch grenade was down, +0x160 and +0x161 the two extra grenade events were down, +0x162 the grenade latch.
        bool IsSwitchGrenadeDown{ false };
        bool IsButton47Down{ false };
        bool IsButton48Down{ false };
        bool IsGrenadeLatched{ false };
    };

    // Ability object of the unit (unit +0x374), as the client asks about it.
    struct AbilityContext
    {
        bool IsPresent{ false };

        // Byte +0x24A of its tag: 0 = hold lane, 1 = button.
        std::uint8_t Kind{ 0 };

        // The kind 1 ability depends on state the port cannot give (setting +0x69 and a tag test): not ported.
        bool NeedsEngine{ false };
    };

    // What the jump assist asks of the unit (0x5df80 sets button bit 14 from it).
    struct JumpAssistContext
    {
        // The feature check and the object check both hold.
        bool Applies{ false };

        // The tag pointer of the unit is not null, and 0x4a0670 gave 1.
        bool HasObject{ false };
        bool IsStateOne{ false };

        // Bit 1 and bit 0xE of the flags at object +0x1D8, the signed counter at object +0xA29 and the tick limit (0x5cc0c).
        bool IsFlagA{ false };
        bool IsFlagB{ false };
        std::int8_t Counter{ 0 };
        std::int32_t LimitHalf{ 0 };
    };

    // Facts of the unit the port may not know. A context says which it knows. A step records which unknown ones it read (VW.9c.1).
    enum class UnitFact : std::uint32_t
    {
        None = 0,
        HasUnit = 1u << 0,
        IsSeated = 1u << 1,
        LookRated = 1u << 2,
        Biped = 1u << 3,
        SeatFlags = 1u << 4,
        AltPitch = 1u << 5,
        ZoomMagnification = 1u << 6,
        MovePenalty = 1u << 7,
        LookOffset = 1u << 8,
        TriggerCurve = 1u << 9,
        AimDamp = 1u << 10,
        ControlLocked = 1u << 11,
        CrouchGate = 1u << 12,
        WeaponCrouchFlag = 1u << 13,
        ForcedCrouch = 1u << 14,
        CrouchToggleBlocked = 1u << 15,
        Carried = 1u << 16,
        Ability = 1u << 17,
        DualWield = 1u << 18,
        AltControl = 1u << 19,
        ZoomItem = 1u << 20,
        WeaponTriggerFlag = 1u << 21,
        FrameOfReference = 1u << 22,
        SelectionEntries = 1u << 23,
        JumpAssist = 1u << 24,
    };

    // Every fact known: what a context built by hand (a test) means.
    inline constexpr std::uint32_t k_AllUnitFacts{ (1u << 25) - 1u };

    // What the client asks about the unit it controls. The engine port fills it (Engine.Control) with the engine functions against the
    // unit it is given.
    struct UnitContext
    {
        // Facts (UnitFact bits) the filler resolved. A fact outside it holds a default, not the truth: a step that reads it says so in
        // ClientOutput::UnknownRead and its result is not comparable with the engine.
        std::uint32_t Known{ k_AllUnitFacts };

        bool HasUnit{ false };

        // The player has the accessibility look rates (player +0x310).
        bool IsLookRated{ false };

        // The object kind of the unit is 0 (biped).
        bool IsBiped{ true };

        // Unit +0x336 and +0x14 are set: the unit sits in a vehicle or a turret.
        bool IsSeated{ false };

        // Seat flag asked of the parent (0x48c3f8) and the parent kind is 2 (0x4ae218): jump and crouch events come from 0xE and 0xF.
        bool IsSeatFlagged{ false };
        bool IsParentKind2{ false };

        // 0x65f0c: the seat takes the alternate copy of the pitch delta.
        bool IsAltPitch{ false };

        // Magnification of the weapon at the zoom level of the client (0x4856b8), 0 if it has none.
        float ZoomMagnification{ 0.0f };

        // Movement scale the weapon in the hands asks, as fractions taken off the two axes (0x4bc51c with the zoom state of the client).
        float MovePenaltyForward{ 0.0f };
        float MovePenaltySide{ 0.0f };

        // The player is pushed off its aim by a flinch (0x1da824): not ported.
        bool IsLookOffset{ false };

        // Bit 27 of the unit tag: the zoom trigger goes through a curve: not ported.
        bool IsTriggerCurve{ false };

        // Aim damping: 1 - tag +0x304 * unit +0x4D4.
        float AimDampTag{ 0.0f };
        float AimDampUnit{ 0.0f };

        // Pitch limits of the unit tag, radians.
        float PitchMin{ -k_PitchLimit };
        float PitchMax{ k_PitchLimit };

        // 0x61830: the controls are locked.
        bool IsControlLocked{ false };

        // 0x4ac810 (biped), 0x4bcae4 (weapon in the hands), 0x4abee8 (forced crouch) and 0x48be48 or kind 0xB9 (no crouch toggle).
        bool IsCrouchGate{ false };
        bool IsWeaponCrouchFlag{ false };
        bool IsForcedCrouch{ false };
        bool IsCrouchToggleBlocked{ false };

        // Carried object (0x48d2d8): its tag is the drop kind (button 16 and zoom reset) or only resets the zoom.
        bool IsCarriedDrop{ false };
        bool IsCarriedZoomReset{ false };

        AbilityContext Ability{};

        // Dual wield (0x48e330), its restrictions byte (0x1d266c: 1 no throw, 2 no zoom, 4 no zoom in) and the alternate control mode
        // (0x1715ac) where the zoom comes from event 0x28.
        bool IsDualWield{ false };
        std::uint8_t DualWieldFlags{ 0 };
        bool IsAltControl{ false };

        // The weapon in the hands (or the seat weapon) is known, and it has no zoom levels (0x4bc624).
        bool HasZoomItem{ false };
        bool IsZoomItemWithoutLevels{ false };

        // 0x4bc978 on the weapon in the hands.
        bool IsWeaponTriggerFlag{ false };

        // The unit has a frame of reference (0x5dddc gave a handle): not ported.
        bool IsFrameOfReference{ false };

        // Entries of the selection list of the unit tag: not ported.
        std::uint32_t SelectionEntries{ 0 };

        JumpAssistContext JumpAssist{};
    };

    // Settings of the user the client uses. Passed in, never read inside (D359).
    struct ClientSettings
    {
        // Aim scale of the user: with zoom (+0x18) and in a seat without zoom (+0x19).
        float AimScaleZoomed{ 1.0f };
        float AimScaleSeated{ 1.0f };

        // Crouch is a toggle (global byte).
        bool IsCrouchToggle{ false };

        // Hold threshold of the control tag in milliseconds (+0x48 seconds * 1000), and the global that also counts melee for the use lane.
        std::int32_t HoldThresholdMs{ 500 };
        bool IsMeleeHoldsUse{ false };

        // Control tag +0x4C: zoom is cancelled when the look delta is larger (0 = 0.8).
        float ZoomCancelThreshold{ 0.0f };

        // Jump timer: setting +0x69 and control tag +0x88 seconds.
        bool IsJumpFlag{ false };
        float JumpTimerSeconds{ 0.0f };

        // Ticks per second of the engine and the seconds of one tick (engine clock +6 and +8).
        std::int16_t TicksPerSecond{ 60 };
        float TickSeconds{ 1.0f / 60.0f };

        // Game system present (in a game, not in a menu), session ready, menu context and input enabled (0x5d6cc).
        bool IsGameSystemPresent{ true };
        bool IsSessionReady{ true };
        bool IsMenuContext{ false };
        bool IsInputEnabled{ true };
    };

    // Branches of 0x5df80 the client did not run because they need state the pure function does not have. A step with any bit set is
    // not comparable with the engine.
    enum class Unsupported : std::uint32_t
    {
        None = 0,
        LookOffset = 1u << 0,
        TriggerCurve = 1u << 1,
        FrameOfReference = 1u << 2,
        SelectionList = 1u << 3,
        AbilityButton = 1u << 4,
        MenuLookScale = 1u << 5,
    };

    // Fields 0x5df80 leaves in its output (param_6, a 0x60 byte structure the caller clears first). The interaction records (+0x34) are
    // not part of it.
    struct ClientOutput
    {
        // +0x00 and +0x04 move axes, +0x08 fire and +0x0C zoom triggers, +0x10 unused here.
        float MoveForward{ 0.0f };
        float MoveSide{ 0.0f };
        float FireTrigger{ 0.0f };
        float ZoomTrigger{ 0.0f };
        float Trigger4{ 0.0f };

        // +0x14 and +0x18 yaw and pitch of the look this call, radians, already scaled.
        float YawDelta{ 0.0f };
        float PitchDelta{ 0.0f };

        // +0x20 buttons, +0x28 event bits (A17 sec. 6: 0 switch weapon, 1 switch grenade, 2 zoom, 3 zoom held, 4 and 5 zoom steps, 6 and 7
        // buttons 17 and 18, 8 reload, 9 throw grenade), +0x2C hold lanes (A17 sec. 8), +0x30 tail value.
        std::uint64_t Buttons{ 0 };
        std::uint32_t Events{ 0 };
        std::uint32_t Lanes{ 0 };
        float Tail30{ 0.0f };

        std::uint32_t Unsupported{ 0 };

        // UnitFact bits the step read while the context did not know them (only the branches that read them count).
        std::uint32_t UnknownRead{ 0 };
    };

    // Why a step did not run.
    enum class ClientStatus : std::uint8_t
    {
        // It ran. The output may still carry Unsupported bits.
        Ran = 0,

        // The input mode of the user is not keyboard and mouse.
        WrongMode,

        // The table is not in the mouse state (+0x6F8 = 0).
        NotMouse,
    };

    // Result of one step of the client.
    struct ClientStep
    {
        ClientStatus Status{ ClientStatus::Ran };
        ClientOutput Output{};
        ClientState State{};
    };

    // Buttons bits 0x5df80 sets (A9 sec. 12.7).
    inline constexpr std::uint64_t k_ButtonCrouch{ 1ull << 0 };
    inline constexpr std::uint64_t k_ButtonJump{ 1ull << 1 };
    inline constexpr std::uint64_t k_ButtonSeatCrouch{ 1ull << 2 };
    inline constexpr std::uint64_t k_ButtonMelee{ 1ull << 11 };
    inline constexpr std::uint64_t k_ButtonZoom{ 1ull << 18 };
    inline constexpr std::uint64_t k_ButtonFire{ 1ull << 30 };
    inline constexpr std::uint64_t k_ButtonThrow{ 1ull << 40 };

    // Event bits of the output (A17 sec. 6).
    inline constexpr std::uint32_t k_EventSwitchWeapon{ 1u << 0 };
    inline constexpr std::uint32_t k_EventSwitchGrenade{ 1u << 1 };
    inline constexpr std::uint32_t k_EventZoomPress{ 1u << 2 };
    inline constexpr std::uint32_t k_EventZoomHeld{ 1u << 3 };
    inline constexpr std::uint32_t k_EventReload{ 1u << 8 };
    inline constexpr std::uint32_t k_EventThrowGrenade{ 1u << 9 };
    inline constexpr std::uint32_t k_EventChordA{ 1u << 10 };
    inline constexpr std::uint32_t k_EventChordB{ 1u << 11 };
}