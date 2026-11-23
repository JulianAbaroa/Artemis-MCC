module Control.Client.System;

import Common.Engine.Type;
import Control.Client.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using Control::Client::Type::AgentControl;
    using Control::Client::Type::AgentControlEvents;
    using Control::Client::Type::AgentEventLink;
    using Control::Client::Type::AgentInput;
    using Control::Client::Type::AgentInputReducer;
    using Control::Client::Type::AgentInputState;
    using Control::Client::Type::ClientOutput;
    using Control::Client::Type::ClientSettings;
    using Control::Client::Type::ClientState;
    using Control::Client::Type::ClientStatus;
    using Control::Client::Type::ClientStep;
    using Control::Client::Type::EventSample;
    using Control::Client::Type::InputEvent;
    using Control::Client::Type::InputFrame;
    using Control::Client::Type::UnitContext;
    using Control::Client::Type::UnitFact;
    using Control::Client::Type::Unsupported;

    using Control::Client::Type::k_AgentControlCount;
    using Control::Client::Type::k_AgentControlEvents;
    using Control::Client::Type::k_AgentControlMask;
    using Control::Client::Type::k_AgentControlNames;
    using Control::Client::Type::k_ButtonCrouch;
    using Control::Client::Type::k_ButtonFire;
    using Control::Client::Type::k_ButtonJump;
    using Control::Client::Type::k_ButtonMelee;
    using Control::Client::Type::k_ButtonSeatCrouch;
    using Control::Client::Type::k_ButtonThrow;
    using Control::Client::Type::k_ButtonZoom;
    using Control::Client::Type::k_EventChordA;
    using Control::Client::Type::k_EventChordB;
    using Control::Client::Type::k_EventReload;
    using Control::Client::Type::k_EventSwitchGrenade;
    using Control::Client::Type::k_EventSwitchWeapon;
    using Control::Client::Type::k_EventThrowGrenade;
    using Control::Client::Type::k_EventZoomHeld;
    using Control::Client::Type::k_EventZoomPress;
    using Control::Client::Type::k_KeyboardMouseMode;
    using Control::Client::Type::k_MillisecondsPerSecond;
    using Control::Client::Type::k_NoZoom;
    using Control::Client::Type::k_PitchLimit;

    constexpr float k_TwoPi{ 6.2831855f };

    // Radians the client turns per unit of the raw mouse delta of the table, before the zoom factor and the aim damp (StepLook passes the
    // delta through).
    constexpr float k_RadiansPerRawLook{ 1.0f };

    // Most milliseconds a held control reports: the table keeps them in a u16.
    constexpr long k_MaxHeldMs{ 65535 };

    // Seconds of look rate the accessibility accumulators hold at most, and the scale of one frame.
    constexpr float k_LookRateLimit{ 7.0f };
    constexpr float k_LookDeltaLimit{ 1.0f };

    // Ms a pause button must be held before it counts as a lane press.
    constexpr std::uint16_t k_PauseHoldMs{ 300 };

    // Default zoom cancel threshold when the control tag has none.
    constexpr float k_DefaultZoomCancel{ 0.8f };

    // Hold threshold of the crouch toggle, in half seconds of ticks (0x5df80: ticks per second / 30 * 15 ticks of 1/60 s).
    constexpr std::int32_t k_CrouchHoldBase{ 0x1E };
    constexpr std::int32_t k_CrouchHoldTicks{ 0xF };
    constexpr std::int32_t k_ChordTicks{ 0x3C };

    // Lanes bits (A17 sec. 8).
    constexpr std::uint32_t k_LaneUseDown{ 20 };
    constexpr std::uint32_t k_LaneUseHeld{ 21 };
    constexpr std::uint32_t k_LaneAbility{ 31 };
    constexpr std::uint32_t k_LanePause{ 24 };

    auto Sample(const InputFrame& frame, InputEvent event) -> const EventSample&
    {
        return frame.Events[static_cast<std::size_t>(event)];
    }

    auto IsDown(const InputFrame& frame, InputEvent event) -> bool
    {
        return Sample(frame, event).IsDown;
    }

    // The engine compares the raw down byte with 1 (0x5d31c): only the first frame of a press counts, not every frame it stays down.
    auto IsPressed(const InputFrame& frame, InputEvent event) -> bool
    {
        return Sample(frame, event).DownCount == 1;
    }

    // Clamp the way the engine does it: below the low value gives the low value, at or above the high value gives the high value.
    auto ClampRange(float value, float low, float high) -> float
    {
        if (value - low < 0.0f) value = low;
        if (0.0f <= value - high) value = high;
        return value;
    }

    template<typename T>
    auto SetMask(T& value, T mask, bool isOn) -> void
    {
        value = isOn ? static_cast<T>(value | mask) : static_cast<T>(value & ~mask);
    }

    template<typename T>
    auto SetBit(T& value, std::uint32_t bit, bool isOn) -> void
    {
        SetMask(value, static_cast<T>(T{ 1 } << bit), isOn);
    }

    // 0x5dc64: the jump press timer and its phase.
    auto StepJumpTimer(ClientState& state, const ClientSettings& settings, bool isJump, float dt) -> void
    {
        float timer{ state.JumpTimer - dt };
        const std::int32_t phase{ state.JumpPhase };
        if (timer <= 0.0f) timer = 0.0f;
        state.JumpTimer = timer;

        if (phase == 0)
        {
            if (isJump)
            {
                state.JumpPhase = 2;
                if (settings.IsJumpFlag)
                {
                    timer = settings.JumpTimerSeconds;
                    state.JumpTimer = timer;
                }
            }
        }
        else if (phase == 1)
        {
            if (!isJump) state.JumpPhase = 0;
        }
        else if (phase == 2)
        {
            if (!isJump) state.JumpPhase = 3;
        }
        else if (phase == 3 && isJump)
        {
            state.JumpPhase = 4;
        }

        if (timer == 0.0f) state.JumpPhase = isJump ? 1 : 0;
    }

    // Records that a branch read a fact of the unit. Only the facts the context does not know count.
    auto Need(const UnitContext& unit, std::uint32_t& unknown, UnitFact fact) -> void
    {
        const std::uint32_t bit{ static_cast<std::uint32_t>(fact) };
        if ((unit.Known & bit) == 0) unknown |= bit;
    }

    // Accessibility look rates of the player (block +0x9C to +0xB4).
    auto StepLookRates(ClientState& state, const InputFrame& frame, const UnitContext& unit, float dt, std::uint32_t& unknown) -> void
    {
        Need(unit, unknown, UnitFact::LookRated);
        if (!unit.IsLookRated)
        {
            state.LookRemainderYaw = 0.0f;
            state.LookRemainderPitch = 0.0f;
            state.LookRateActive = 0;
            return;
        }

        state.LookRateActive = 1;
        if (std::fabs(dt) < 0.0001f) return;

        const float yaw{ ClampRange((1.0f / dt) * frame.YawDelta + state.LookRemainderYaw, -k_LookRateLimit, k_LookRateLimit) };
        const float pitch{ ClampRange((1.0f / dt) * frame.PitchDelta + state.LookRemainderPitch, -k_LookRateLimit, k_LookRateLimit) };
        const float yawRate{ ClampRange(yaw, -k_LookDeltaLimit, k_LookDeltaLimit) };
        const float pitchRate{ ClampRange(pitch, -k_LookDeltaLimit, k_LookDeltaLimit) };

        state.LookRateYaw = yawRate;
        state.LookRatePitch = pitchRate;
        state.LookRemainderYaw = yaw - yawRate;
        state.LookRemainderPitch = pitch - pitchRate;
    }

    // 0x618f0: the factor the look delta gets from the zoom (or the seat) of the unit.
    auto GetAimFactor(const ClientState& state, const UnitContext& unit, const ClientSettings& settings, std::uint32_t& unknown) -> float
    {
        float factor{ 1.0f };
        if (!unit.HasUnit) return factor;

        if (state.ZoomLevel == k_NoZoom)
        {
            if (unit.IsSeated) factor = settings.AimScaleSeated;
        }
        else if (Need(unit, unknown, UnitFact::ZoomMagnification), 0.0f < unit.ZoomMagnification)
        {
            factor = settings.AimScaleZoomed / unit.ZoomMagnification;
        }

        return factor;
    }

    // Axis of an agent: finite and inside [-1, 1].
    auto SanitizeAxis(float value) -> float
    {
        return std::isfinite(value) ? std::clamp(value, -1.0f, 1.0f) : 0.0f;
    }

    auto SanitizeDelta(float value) -> float
    {
        return std::isfinite(value) ? value : 0.0f;
    }

    // Per tick look of an agent cut to the turn limit. A limit of zero or less (or NaN) cuts nothing.
    auto LimitTurn(float perTick, float turnLimit) -> float
    {
        return turnLimit > 0.0f ? std::clamp(perTick, -turnLimit, turnLimit) : perTick;
    }

    // Whether a control is down at tick tick of a window (D362). startDown: it was down at the end of the previous window.
    // note: A press of a control that was down is up at tick 0 and down from tick 1 (a tap, only at tick 1).
    auto IsAgentControlDown(bool isLevel, bool isEdge, bool startDown, std::int32_t tick) -> bool
    {
        if (isLevel) return !(isEdge && startDown) || 1 <= tick;
        if (isEdge) return tick == (startDown ? 1 : 0);
        return false;
    }

    // Look part of 0x5df80 with the mouse: the delta of the table, scaled by the zoom and the unit, to this call.
    auto StepLook(ClientStep& step, const InputFrame& frame, const UnitContext& unit, const ClientSettings& settings, float dt,
        std::uint32_t& unknown) -> void
    {
        ClientOutput& output{ step.Output };
        ClientState& state{ step.State };

        if (!settings.IsInputEnabled)
        {
            output.YawDelta = 0.0f;
            output.PitchDelta = 0.0f;
            output.Tail30 = 0.0f;
            return;
        }

        Need(unit, unknown, UnitFact::AltPitch);
        const float pitchSource{ unit.IsAltPitch ? frame.PitchDeltaAlt : frame.PitchDelta };

        float yawRate{ 0.0f };
        float pitchRate{ 0.0f };
        if (0.0f < dt)
        {
            yawRate = frame.YawDelta / dt;
            pitchRate = pitchSource / dt;
        }

        // note: Outside a game the engine scales by the look speed of the user (menus). Not used by an agent.
        if (!settings.IsGameSystemPresent) output.Unsupported |= static_cast<std::uint32_t>(Unsupported::MenuLookScale);

        const float factor{ GetAimFactor(state, unit, settings, unknown) };
        float pitchOut{ factor * pitchRate };
        float yawOut{ factor * yawRate };
        const float yawAsked{ yawRate * dt };
        const float pitchAsked{ dt * pitchRate };

        if (unit.HasUnit)
        {
            Need(unit, unknown, UnitFact::AimDamp);
            const float damp{ 1.0f - (unit.AimDampTag * unit.AimDampUnit) };
            pitchOut = damp * pitchOut;
            yawOut = damp * yawOut;
        }

        // note: With the mouse the aim assist blend is off, whatever the unit.
        state.AimAssistActive = 0;

        output.YawDelta = yawOut * dt;
        output.PitchDelta = dt * pitchOut;
        output.LookYawRequested = yawAsked;
        output.LookPitchRequested = pitchAsked;
        output.Tail30 = frame.Tail6D0;
    }

    // Crouch toggle or hold: the state of the crouch of the client block.
    auto StepCrouch(ClientState& state, const InputFrame& frame, const UnitContext& unit, const ClientSettings& settings, float dt,
        std::uint32_t& unknown) -> void
    {
        Need(unit, unknown, UnitFact::ControlLocked);
        if (unit.IsControlLocked) return;

        if (unit.HasUnit)
        {
            Need(unit, unknown, UnitFact::Biped);
            if (unit.IsBiped) Need(unit, unknown, UnitFact::CrouchGate);
            Need(unit, unknown, UnitFact::WeaponCrouchFlag);
        }

        const bool isGate{ unit.HasUnit && unit.IsBiped && unit.IsCrouchGate };
        const bool isWeaponFlag{ unit.HasUnit && unit.IsWeaponCrouchFlag };
        const bool isCrouch{ IsDown(frame, InputEvent::Crouch) };

        if (isWeaponFlag)
        {
            state.IsCrouchToggled = false;
            return;
        }

        // note: The toggle flips on the first frame of the press only, a hold sets the crouch on that frame and clears it when released.
        const bool isCrouchPress{ IsPressed(frame, InputEvent::Crouch) };
        if (settings.IsCrouchToggle && !isGate)
        {
            if (isCrouchPress) state.IsCrouchToggled = !state.IsCrouchToggled;

            const std::int32_t holdTicks{ (settings.TicksPerSecond / k_CrouchHoldBase) * k_CrouchHoldTicks };
            const float holdSeconds{ static_cast<float>(holdTicks) * settings.TickSeconds };
            if (!isCrouch && holdSeconds <= state.CrouchHeldSeconds) state.IsCrouchToggled = false;

            state.CrouchHeldSeconds = isCrouch ? (dt + state.CrouchHeldSeconds) : 0.0f;
            return;
        }

        if (isCrouchPress) state.IsCrouchToggled = true;
        else if (!isCrouch) state.IsCrouchToggled = false;
    }

    // Zoom events: press, hold, step in and step out, with the limits of dual wield and of weapons without zoom.
    auto StepZoom(ClientOutput& output, const ClientState& state, const InputFrame& frame, const UnitContext& unit,
        const ClientSettings& settings, std::uint32_t& unknown) -> void
    {
        if (unit.IsSeated) return;

        // note: The held bit is any non zero down byte, the press and the two steps are the first frame only (0x5fefd: byte == 1).
        bool isHeld{ false };
        bool isPress{ false };
        bool isIn{ false };
        bool isOut{ false };
        Need(unit, unknown, UnitFact::AltControl);
        if (!unit.IsAltControl)
        {
            isHeld = IsDown(frame, InputEvent::Zoom);
            isPress = IsPressed(frame, InputEvent::Zoom);
            isIn = IsPressed(frame, InputEvent::ZoomIn);
            isOut = IsPressed(frame, InputEvent::ZoomOut);
        }
        else
        {
            isHeld = IsDown(frame, InputEvent::ZoomAlt);
            isPress = IsPressed(frame, InputEvent::ZoomAlt);
        }

        if (isPress || isIn) Need(unit, unknown, UnitFact::DualWield);
        if (isPress && unit.IsDualWield) isPress = (unit.DualWieldFlags & 2) == 0;
        if (isIn && unit.IsDualWield && (unit.DualWieldFlags & 4) != 0) isIn = false;

        if (isPress && state.ZoomLevel == k_NoZoom)
        {
            if (frame.RawValue0 > 127.0f || frame.RawValue1 > 127.0f)
            {
                isPress = false;
                isOut = false;
                isIn = false;
            }

            Need(unit, unknown, UnitFact::ZoomItem);
            if (unit.HasZoomItem && unit.IsZoomItemWithoutLevels)
            {
                const float threshold{ settings.ZoomCancelThreshold == 0.0f ? k_DefaultZoomCancel : settings.ZoomCancelThreshold };
                if (threshold * threshold < (frame.PitchDelta * frame.PitchDelta) + (frame.YawDelta * frame.YawDelta))
                {
                    isIn = false;
                    isPress = false;
                    isOut = false;
                }
            }
        }

        SetMask(output.Events, k_EventZoomPress, isPress);
        SetBit(output.Events, 4, isIn);
        SetBit(output.Events, 5, isOut);
        SetMask(output.Events, k_EventZoomHeld, isHeld);

        // note: A zoom press that was asked for while a drop is pending clears the ability bit.
        if (isPress && (output.Buttons & (1ull << 16)) != 0) output.Buttons &= ~(1ull << 8);
    }

    // Lanes of the hold buttons: a down bit, and a held bit once held for the threshold (A17 sec. 8).
    auto StepLanes(ClientOutput& output, const InputFrame& frame, const ClientSettings& settings) -> void
    {
        const std::int32_t threshold{ settings.HoldThresholdMs };
        auto isHeld = [&](InputEvent event) {
            return threshold <= static_cast<std::int32_t>(Sample(frame, event).HeldMs);
        };

        output.Lanes &= 0xFFFFFFFCu;

        SetBit(output.Lanes, k_LaneUseDown, IsDown(frame, InputEvent::Use));
        const bool isUseHeld{ isHeld(InputEvent::Use) || (isHeld(InputEvent::Melee) && settings.IsMeleeHoldsUse) };
        SetBit(output.Lanes, k_LaneUseHeld, isUseHeld);

        SetBit(output.Lanes, 4, IsDown(frame, InputEvent::LaneMelee));
        SetBit(output.Lanes, 5, isHeld(InputEvent::LaneMelee));
        SetBit(output.Lanes, 8, IsDown(frame, InputEvent::SwitchWeapon));
        SetBit(output.Lanes, 9, isHeld(InputEvent::SwitchWeapon));
        SetBit(output.Lanes, 12, IsDown(frame, InputEvent::Ability));
        SetBit(output.Lanes, 13, isHeld(InputEvent::Ability));
        SetBit(output.Lanes, 16, IsDown(frame, InputEvent::LaneButton));
        SetBit(output.Lanes, 17, isHeld(InputEvent::LaneButton));
        SetBit(output.Lanes, 25, IsDown(frame, InputEvent::LaneSwitchAlt));
        SetBit(output.Lanes, 27, IsDown(frame, InputEvent::LaneHold));
        SetBit(output.Lanes, 29, isHeld(InputEvent::LaneHold));
    }

    // Button bits the events set directly, the seat variants and the weapon trigger flag.
    auto StepButtons(ClientOutput& output, const InputFrame& frame, const UnitContext& unit, std::uint32_t& unknown) -> void
    {
        std::uint64_t& buttons{ output.Buttons };
        const bool isSeated{ unit.IsSeated };
        const bool isZoomValue{ IsDown(frame, InputEvent::ZoomValue) };

        SetMask(buttons, k_ButtonFire, IsDown(frame, InputEvent::Fire));
        SetBit(buttons, 35, isSeated && isZoomValue);
        SetBit(buttons, 31, isSeated && isZoomValue);

        bool isBit2{ false };
        bool isBit20{ false };
        Need(unit, unknown, UnitFact::SeatFlags);
        if (!(unit.IsParentKind2 && unit.IsSeatFlagged))
        {
            isBit2 = isSeated && IsDown(frame, InputEvent::SeatCrouch);
            isBit20 = isSeated && IsDown(frame, InputEvent::SeatJump);
        }
        else
        {
            isBit2 = IsDown(frame, InputEvent::SeatCrouchAlt);
            isBit20 = IsDown(frame, InputEvent::SeatJumpAlt);
        }

        SetMask(buttons, k_ButtonSeatCrouch, isBit2);
        SetBit(buttons, 20, isBit20);
        SetBit(buttons, 15, isSeated && isZoomValue);

        if (IsDown(frame, InputEvent::Throw)) Need(unit, unknown, UnitFact::DualWield);
        const bool isThrow{ IsDown(frame, InputEvent::Throw) && (!unit.IsDualWield || (unit.DualWieldFlags & 1) == 0) };
        SetMask(buttons, k_ButtonThrow, isThrow);
        SetMask(buttons, k_ButtonZoom, isZoomValue);
        SetBit(buttons, 3, IsDown(frame, InputEvent::Button16));
        SetBit(buttons, 34, IsDown(frame, InputEvent::Button16));
        SetBit(buttons, 39, IsDown(frame, InputEvent::Button16));
        SetMask(buttons, k_ButtonJump, !isSeated && IsDown(frame, InputEvent::Jump));
        SetBit(buttons, 10, IsDown(frame, InputEvent::Button11));
        SetMask(buttons, k_ButtonMelee, IsDown(frame, InputEvent::Melee));
        SetBit(buttons, 47, IsDown(frame, InputEvent::Button46));
        SetBit(output.Events, 12, IsDown(frame, InputEvent::Button38));

        if (unit.HasUnit) Need(unit, unknown, UnitFact::WeaponTriggerFlag);
        if (unit.HasUnit) SetBit(buttons, 12, unit.IsWeaponTriggerFlag && IsDown(frame, InputEvent::Fire));
    }

    // Switch grenade, with the two extra events and the latch that the client block keeps.
    auto StepGrenade(ClientOutput& output, ClientState& state, const InputFrame& frame) -> void
    {
        const bool isSwitch{ IsDown(frame, InputEvent::SwitchGrenade) };
        const bool isButton47{ IsDown(frame, InputEvent::Button47) };
        const bool isButton48{ IsDown(frame, InputEvent::Button48) };

        const bool isReleased{ !isSwitch && state.IsSwitchGrenadeDown };
        const bool isReleased47{ !isButton47 && state.IsButton47Down };
        const bool isReleased48{ !isButton48 && state.IsButton48Down };

        const bool isAny{ isReleased || isReleased47 || isReleased48 };
        SetMask(output.Events, k_EventSwitchGrenade, isAny && !state.IsGrenadeLatched);

        state.IsSwitchGrenadeDown = isSwitch;
        state.IsButton47Down = isButton47;
        state.IsButton48Down = isButton48;
        if (isReleased) state.IsGrenadeLatched = false;
    }
}

namespace Control::Client::System
{
    auto SanitizeAgentInput(const AgentInput& input) -> AgentInput
    {
        return AgentInput{
            .MoveForward = SanitizeAxis(input.MoveForward),
            .MoveLeft = SanitizeAxis(input.MoveLeft),
            .YawDelta = SanitizeDelta(input.YawDelta),
            .PitchDelta = SanitizeDelta(input.PitchDelta),
            .Level = input.Level & k_AgentControlMask,
            .Edge = input.Edge & k_AgentControlMask };
    }

    auto MakeAgentFrame(const AgentInput& input, std::int32_t tickInWindow, std::int32_t windowTicks, const ClientSettings& settings,
        float turnLimit, AgentInputState& state) -> InputFrame
    {
        const AgentInput clean{ SanitizeAgentInput(input) };
        const std::int32_t length{ (std::max)(windowTicks, 1) };
        const std::int32_t tick{ (std::max)(tickInWindow, 0) };
        if (tick == 0) state.StartMask = state.DownMask;

        InputFrame frame{};
        frame.MoveForward = clean.MoveForward;
        frame.MoveSide = clean.MoveLeft;

        const float yaw{ LimitTurn(clean.YawDelta / static_cast<float>(length), turnLimit) };
        const float pitch{ LimitTurn(clean.PitchDelta / static_cast<float>(length), turnLimit) };
        frame.YawDelta = yaw / k_RadiansPerRawLook;
        frame.PitchDelta = pitch / k_RadiansPerRawLook;
        frame.PitchDeltaAlt = frame.PitchDelta;

        std::uint32_t downMask{ 0 };
        for (std::uint32_t control{ 0 }; control < k_AgentControlCount; ++control)
        {
            const std::uint32_t bit{ 1u << control };
            const bool isDown{ IsAgentControlDown((clean.Level & bit) != 0, (clean.Edge & bit) != 0, (state.StartMask & bit) != 0, tick) };
            if (!isDown)
            {
                state.DownCount[control] = 0;
                state.TicksDown[control] = 0;
                continue;
            }

            downMask |= bit;
            if (state.DownCount[control] < 255) ++state.DownCount[control];
            if (state.TicksDown[control] < (std::numeric_limits<std::uint32_t>::max)()) ++state.TicksDown[control];

            const float heldMs{ static_cast<float>(state.TicksDown[control]) * settings.TickSeconds *
                static_cast<float>(k_MillisecondsPerSecond) };
            const EventSample sample{ .IsDown = true, .DownCount = state.DownCount[control],
                .HeldMs = static_cast<std::uint16_t>(std::clamp(std::lround(heldMs), 0L, k_MaxHeldMs)) };

            const AgentControlEvents& events{ k_AgentControlEvents[control] };
            auto write = [&](const AgentEventLink& link) {
                EventSample& slot{ frame.Events[static_cast<std::size_t>(link.Event)] };
                slot = sample;
                slot.Value = link.Value;
            };
            write(events.Primary);
            for (std::uint8_t i{ 0 }; i < events.SecondaryCount; ++i) write(events.Secondary[i]);
        }

        state.DownMask = downMask;
        return frame;
    }

    auto FindAgentControl(std::string_view name) -> std::optional<AgentControl>
    {
        const auto found{ std::ranges::find(k_AgentControlNames, name) };
        if (found == k_AgentControlNames.end()) return std::nullopt;

        return static_cast<AgentControl>(found - k_AgentControlNames.begin());
    }

    auto ParseAgentInputWords(std::span<const std::string> words, AgentInput& input, float& turnLimit) -> std::string
    {
        AgentInput parsed{ input };
        float parsedTurn{ turnLimit };
        std::array<bool, 5> isSeen{};

        for (const std::string& word : words)
        {
            const std::size_t equals{ word.find('=') };
            if (equals == std::string::npos) return std::format("'{}' is not key=value", word);

            const std::string_view key{ std::string_view{ word }.substr(0, equals) };
            const std::string_view value{ std::string_view{ word }.substr(equals + 1) };

            std::size_t slot{ 0 };
            if (key == "move") slot = 0;
            else if (key == "look") slot = 1;
            else if (key == "level") slot = 2;
            else if (key == "edge") slot = 3;
            else if (key == "turn") slot = 4;
            else return std::format("unknown key '{}'", key);

            if (isSeen[slot]) return std::format("'{}' is given twice", key);
            isSeen[slot] = true;

            if (slot == 2 || slot == 3)
            {
                std::uint32_t mask{ 0 };
                if (value != "-")
                {
                    for (std::size_t start{ 0 }; start <= value.size();)
                    {
                        const std::size_t comma{ (std::min)(value.find(',', start), value.size()) };
                        const std::string_view name{ value.substr(start, comma - start) };
                        const std::optional<AgentControl> control{ FindAgentControl(name) };
                        if (!control) return std::format("unknown control '{}'", name);

                        mask |= 1u << static_cast<std::uint32_t>(*control);
                        start = comma + 1;
                    }
                }

                (slot == 2 ? parsed.Level : parsed.Edge) = mask;
                continue;
            }

            std::array<float, 2> numbers{};
            const std::size_t count{ slot == 4 ? 1u : 2u };
            std::string_view rest{ value };
            for (std::size_t index{ 0 }; index < count; ++index)
            {
                const std::size_t comma{ (std::min)(rest.find(','), rest.size()) };
                const std::string_view text{ rest.substr(0, comma) };
                const auto [end, error]{ std::from_chars(text.data(), text.data() + text.size(), numbers[index]) };
                if (text.empty() || error != std::errc{} || end != text.data() + text.size() || !std::isfinite(numbers[index]))
                {
                    return std::format("'{}' takes {} finite number{} separated by commas", key, count, count == 1 ? "" : "s");
                }

                const bool isLast{ index + 1 == count };
                if (isLast != (comma == rest.size())) return std::format("'{}' takes {} number{} separated by commas", key, count, count == 1 ? "" : "s");
                rest = rest.substr((std::min)(comma + 1, rest.size()));
            }

            if (slot == 0)
            {
                if (std::abs(numbers[0]) > 1.0f || std::abs(numbers[1]) > 1.0f) return "move axes must be in [-1, 1]";
                parsed.MoveForward = numbers[0];
                parsed.MoveLeft = numbers[1];
            }
            else if (slot == 1)
            {
                parsed.YawDelta = numbers[0];
                parsed.PitchDelta = numbers[1];
            }
            else
            {
                if (numbers[0] < 0.0f) return "turn must be 0 or more";
                parsedTurn = numbers[0];
            }
        }

        input = parsed;
        turnLimit = parsedTurn;
        return {};
    }

    auto AddAgentFrame(AgentInputReducer& reducer, const InputFrame& frame, float lookYaw, float lookPitch) -> void
    {
        std::uint32_t level{ 0 };
        for (std::uint32_t control{ 0 }; control < k_AgentControlCount; ++control)
        {
            const EventSample& primary{ Sample(frame, k_AgentControlEvents[control].Primary.Event) };
            if (primary.IsDown) level |= 1u << control;
            if (primary.DownCount == 1) reducer.Edge |= 1u << control;
        }

        reducer.Level = level;
        reducer.MoveForward = frame.MoveForward;
        reducer.MoveLeft = frame.MoveSide;
        reducer.LookYaw += lookYaw;
        reducer.LookPitch += lookPitch;
    }

    auto CloseAgentInput(AgentInputReducer& reducer) -> AgentInput
    {
        const AgentInput result{ SanitizeAgentInput(AgentInput{ .MoveForward = reducer.MoveForward, .MoveLeft = reducer.MoveLeft,
            .YawDelta = reducer.LookYaw, .PitchDelta = reducer.LookPitch, .Level = reducer.Level, .Edge = reducer.Edge }) };
        reducer = AgentInputReducer{ .Level = reducer.Level };
        return result;
    }

    auto StepClient(const InputFrame& frame, const ClientState& state, const UnitContext& unit, const ClientSettings& settings, float dt)
        -> ClientStep
    {
        ClientStep step{ .State = state };
        if (frame.Mode != k_KeyboardMouseMode)
        {
            step.Status = ClientStatus::WrongMode;
            return step;
        }

        if (!frame.IsMouse)
        {
            step.Status = ClientStatus::NotMouse;
            return step;
        }

        ClientOutput& output{ step.Output };
        ClientState& next{ step.State };

        std::uint32_t& unknown{ output.UnknownRead };
        Need(unit, unknown, UnitFact::HasUnit);
        Need(unit, unknown, UnitFact::IsSeated);

        StepJumpTimer(next, settings, IsDown(frame, InputEvent::Jump), dt);
        StepLookRates(next, frame, unit, dt, unknown);

        SetMask(output.Events, k_EventReload, IsDown(frame, InputEvent::Reload));
        SetMask(output.Events, k_EventThrowGrenade, IsDown(frame, InputEvent::ThrowGrenade));

        output.MoveForward = frame.MoveForward;
        output.MoveSide = frame.MoveSide;
        Need(unit, unknown, UnitFact::LookOffset);
        if (unit.IsLookOffset) output.Unsupported |= static_cast<std::uint32_t>(Unsupported::LookOffset);

        if (unit.HasUnit)
        {
            Need(unit, unknown, UnitFact::MovePenalty);

            output.MoveForward = (1.0f - unit.MovePenaltyForward) * output.MoveForward;
            output.MoveSide = (1.0f - unit.MovePenaltySide) * output.MoveSide;

            // note: The boost goes through the tag function of the unit, and a flagged tag also pushes forward. It runs after the move
            // penalty, as 0x5df80 does.
            Need(unit, unknown, UnitFact::TriggerCurve);
            if (unit.IsTriggerCurve)
            {
                Need(unit, unknown, UnitFact::BoostAltEvent);
                Need(unit, unknown, UnitFact::TriggerCurveValue);
                Need(unit, unknown, UnitFact::TriggerCurveForward);

                const float boost{ unit.TriggerCurveValue };
                // note: As 0x5ebef (maxss with 0, then the compare with 1): NaN and -0 give +0, unlike std::max.
                float trigger{ boost > 0.0f ? boost : 0.0f };
                if (trigger - 1.0f >= 0.0f) trigger = 1.0f;
                output.Trigger4 = trigger;
                if (unit.IsTriggerCurveForward && boost > 0.0f)
                {
                    output.MoveForward = output.MoveForward < 0.0f ? output.MoveForward + 1.0f : 1.0f;
                }
            }
        }

        StepLook(step, frame, unit, settings, dt, unknown);

        if (unit.HasUnit)
        {
            Need(unit, unknown, UnitFact::Biped);
            if (unit.IsBiped)
            {
                Need(unit, unknown, UnitFact::FrameOfReference);
                if (unit.IsFrameOfReference) output.Unsupported |= static_cast<std::uint32_t>(Unsupported::FrameOfReference);
                else next.FrameHandle = k_NoHandle;
            }

            next.SelectionIndex = k_NoHandle;
            Need(unit, unknown, UnitFact::SelectionEntries);
            if (unit.SelectionEntries != 0) output.Unsupported |= static_cast<std::uint32_t>(Unsupported::SelectionList);
        }

        StepCrouch(next, frame, unit, settings, dt, unknown);
        SetMask(output.Buttons, k_ButtonCrouch, next.IsCrouchToggled);
        if (unit.HasUnit && unit.IsBiped) Need(unit, unknown, UnitFact::ForcedCrouch);
        if (unit.HasUnit && unit.IsBiped && unit.IsForcedCrouch) output.Buttons |= k_ButtonCrouch;

        if (!unit.IsSeated)
        {
            if (unit.HasUnit)
            {
                Need(unit, unknown, UnitFact::Carried);
                if (unit.IsCarriedDrop)
                {
                    output.Buttons |= 1ull << 16;
                    next.ZoomLevel = k_NoZoom;
                }

                if (unit.IsCarriedZoomReset) next.ZoomLevel = k_NoZoom;
            }

            if ((output.Buttons & (1ull << 16)) != 0)
            {
                output.Buttons &= ~k_ButtonCrouch;
                next.IsCrouchToggled = false;
            }
        }

        if (unit.HasUnit) Need(unit, unknown, UnitFact::Ability);
        if (unit.HasUnit && unit.Ability.IsPresent)
        {
            if (unit.Ability.Kind == 0)
            {
                if (IsPressed(frame, InputEvent::Ability)) output.Lanes |= 1u << k_LaneAbility;
            }
            else if (unit.Ability.Kind == 1)
            {
                if (unit.Ability.NeedsEngine && settings.IsJumpFlag) output.Unsupported |= static_cast<std::uint32_t>(Unsupported::AbilityButton);
                else SetBit(output.Buttons, 8, IsDown(frame, InputEvent::Ability));
            }
        }

        StepZoom(output, next, frame, unit, settings, unknown);

        output.FireTrigger = Sample(frame, InputEvent::Fire).Value;
        output.ZoomTrigger = Sample(frame, InputEvent::ZoomValue).Value;

        if (!settings.IsSessionReady && settings.IsMenuContext)
        {
            const EventSample& pause{ Sample(frame, InputEvent::Pause) };
            if (pause.IsDown && pause.HeldMs > k_PauseHoldMs) output.Lanes |= 1u << k_LanePause;
        }

        SetBit(output.Events, 6, IsDown(frame, InputEvent::Button17));
        SetBit(output.Events, 7, IsDown(frame, InputEvent::Button18));

        StepButtons(output, frame, unit, unknown);

        if (IsDown(frame, InputEvent::Use) && IsDown(frame, InputEvent::LaneMelee) && IsDown(frame, InputEvent::Zoom))
        {
            const std::int32_t chord{ (settings.TicksPerSecond / k_CrouchHoldBase) * k_ChordTicks };
            if (static_cast<std::int32_t>(IsDown(frame, InputEvent::Chord19)) == chord) output.Events |= k_EventChordA;
            if (static_cast<std::int32_t>(IsDown(frame, InputEvent::Chord1A)) == chord) output.Events |= k_EventChordB;
        }

        StepGrenade(output, next, frame);

        if (settings.IsCrouchToggle && next.IsCrouchToggled)
        {
            if ((output.Buttons & k_ButtonJump) != 0) next.IsCrouchToggled = false;
            if (unit.HasUnit)
            {
                Need(unit, unknown, UnitFact::CrouchToggleBlocked);
                if ((output.Buttons & (1ull << 12)) != 0) Need(unit, unknown, UnitFact::WeaponTriggerFlag);
                if (unit.IsCrouchToggleBlocked) next.IsCrouchToggled = false;
                if ((output.Buttons & (1ull << 12)) != 0 && unit.IsWeaponTriggerFlag) next.IsCrouchToggled = false;
            }
        }

        StepLanes(output, frame, settings);

        if (!IsDown(frame, InputEvent::SwitchWeapon))
        {
            if (next.IsSwitchWeaponDown && !next.IsSwitchWeaponLatched) output.Events |= k_EventSwitchWeapon;
            next.IsSwitchWeaponLatched = false;
            next.IsSwitchWeaponDown = false;
        }
        else
        {
            next.IsSwitchWeaponDown = true;
        }

        Need(unit, unknown, UnitFact::JumpAssist);
        if (unit.JumpAssist.Applies)
        {
            bool isOn{ false };
            const auto& assist{ unit.JumpAssist };
            if (assist.HasObject && IsDown(frame, InputEvent::Jump) && assist.IsStateOne)
            {
                if (assist.IsFlagA && assist.Counter < assist.LimitHalf * 2) isOn = true;
                else isOn = assist.IsFlagB;
            }

            SetBit(output.Buttons, 14, isOn);
        }

        return step;
    }

    auto IntegrateAim(const ClientState& state, float yawDelta, float pitchDelta, const UnitContext& unit) -> ClientState
    {
        ClientState next{ state };

        float yaw{ state.Yaw + yawDelta };
        while (yaw < 0.0f) yaw += k_TwoPi;
        while (k_TwoPi < yaw) yaw -= k_TwoPi;
        next.Yaw = yaw;

        const float low{ (std::max)(unit.PitchMin, -k_PitchLimit) };
        const float high{ (std::min)(unit.PitchMax, k_PitchLimit) };
        next.Pitch = (std::min)((std::max)(state.Pitch + pitchDelta, low), high);
        return next;
    }
}