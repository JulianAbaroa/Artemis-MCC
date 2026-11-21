export module Engine.Control.Input.System:ClientFrame;

import Control.Client.Type;
import Engine.Control.Input.Type;
import std;

namespace
{
    using Control::Client::Type::ClientState;
    using Control::Client::Type::ClientStep;
    using Control::Client::Type::EventSample;
    using Control::Client::Type::InputFrame;
    using Control::Client::Type::k_EventCount;
    using Control::Client::Type::k_KeyboardMouseMode;

    using Engine::Control::Input::Type::ClientDifferences;
    using Engine::Control::Input::Type::ClientStateField;
    using Engine::Control::Input::Type::k_ClientAimAssistOffset;
    using Engine::Control::Input::Type::k_ClientBlockWindow;
    using Engine::Control::Input::Type::k_ClientCrouchHeldOffset;
    using Engine::Control::Input::Type::k_ClientCrouchOffset;
    using Engine::Control::Input::Type::k_ClientFrameHandleOffset;
    using Engine::Control::Input::Type::k_ClientGrenadeOffset;
    using Engine::Control::Input::Type::k_ClientJumpTimerOffset;
    using Engine::Control::Input::Type::k_ClientLookRateActiveOffset;
    using Engine::Control::Input::Type::k_ClientLookRateYawOffset;
    using Engine::Control::Input::Type::k_ClientLookRemainderYawOffset;
    using Engine::Control::Input::Type::k_ClientOutputSize;
    using Engine::Control::Input::Type::k_ClientSelectionIndexOffset;
    using Engine::Control::Input::Type::k_ClientSelectionKindOffset;
    using Engine::Control::Input::Type::k_ClientSwitchWeaponOffset;
    using Engine::Control::Input::Type::k_ClientYawOffset;
    using Engine::Control::Input::Type::k_ClientZoomLevelOffset;
    using Engine::Control::Input::Type::k_EventDownOffset;
    using Engine::Control::Input::Type::k_EventFlagsOffset;
    using Engine::Control::Input::Type::k_EventHeldOffset;
    using Engine::Control::Input::Type::k_EventLinkAny;
    using Engine::Control::Input::Type::k_EventLinkAnyOffset;
    using Engine::Control::Input::Type::k_EventLinkOffset;
    using Engine::Control::Input::Type::k_EventValueOffset;
    using Engine::Control::Input::Type::k_InputEventSize;
    using Engine::Control::Input::Type::k_InputEventsOffset;
    using Engine::Control::Input::Type::k_InputMouseFlagOffset;
    using Engine::Control::Input::Type::k_InputMoveForwardOffset;
    using Engine::Control::Input::Type::k_InputMoveSideOffset;
    using Engine::Control::Input::Type::k_InputPitchDeltaAltOffset;
    using Engine::Control::Input::Type::k_InputPitchDeltaOffset;
    using Engine::Control::Input::Type::k_InputTableSize;
    using Engine::Control::Input::Type::k_InputTailOffset;
    using Engine::Control::Input::Type::k_InputYawDeltaOffset;
    using Engine::Control::Input::Type::k_OutputButtonsOffset;
    using Engine::Control::Input::Type::k_OutputEventsOffset;
    using Engine::Control::Input::Type::k_OutputFireTriggerOffset;
    using Engine::Control::Input::Type::k_OutputLanesOffset;
    using Engine::Control::Input::Type::k_OutputLookOffset;
    using Engine::Control::Input::Type::k_OutputMoveOffset;
    using Engine::Control::Input::Type::k_OutputTailOffset;
    using Engine::Control::Input::Type::k_OutputTrigger4Offset;
    using Engine::Control::Input::Type::k_OutputZoomTriggerOffset;

    template<typename T>
    auto ReadAt(std::span<const std::uint8_t> bytes, std::size_t offset) -> T
    {
        T value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(T));
        return value;
    }

    // The same test as 0x5d31c, 0x5d358 and 0x5d374: an entry that is blocked, or whose link does not match, reads as zero.
    auto ReadEntry(std::span<const std::uint8_t> table, std::size_t offset) -> EventSample
    {
        const bool isBlocked{ (ReadAt<std::uint8_t>(table, offset + k_EventFlagsOffset) & 1) != 0 };
        const std::uint32_t link{ ReadAt<std::uint32_t>(table, offset + k_EventLinkOffset) };
        const std::uint32_t linkAny{ ReadAt<std::uint32_t>(table, offset + k_EventLinkAnyOffset) };
        if (isBlocked || (linkAny != k_EventLinkAny && linkAny != link)) return EventSample{};

        return EventSample{ .IsDown = ReadAt<std::uint8_t>(table, offset + k_EventDownOffset) != 0,
            .HeldMs = ReadAt<std::uint16_t>(table, offset + k_EventHeldOffset), .Value = ReadAt<float>(table, offset + k_EventValueOffset) };
    }

    auto IsSame(float a, float b) -> bool
    {
        return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
    }
}

export namespace Engine::Control::Input::System
{
    // Builds what the client reads from the event table of an input user (A17 sec. 9): the events of the keyboard and mouse mode, with
    // the entries that read as zero already resolved, and the tail of the table.
    // note: The mode is taken from the mouse flag of the table (+0x6F8), because the address of g_InputProcessing_Mode is not known.
    auto ReadInputFrame(std::span<const std::uint8_t, k_InputTableSize> table) -> InputFrame
    {
        InputFrame frame{};
        frame.IsMouse = ReadAt<std::uint8_t>(table, k_InputMouseFlagOffset) != 0;
        frame.Mode = frame.IsMouse ? k_KeyboardMouseMode : std::uint8_t{ 0 };

        for (std::size_t event{ 0 }; event < k_EventCount; ++event)
        {
            frame.Events[event] = ReadEntry(table, k_InputEventsOffset + (event * k_InputEventSize));
        }

        frame.RawValue0 = ReadEntry(table, 0).Value;
        frame.RawValue1 = ReadEntry(table, k_InputEventSize).Value;
        frame.MoveForward = ReadAt<float>(table, k_InputMoveForwardOffset);
        frame.MoveSide = ReadAt<float>(table, k_InputMoveSideOffset);
        frame.YawDelta = ReadAt<float>(table, k_InputYawDeltaOffset);
        frame.PitchDelta = ReadAt<float>(table, k_InputPitchDeltaOffset);
        frame.PitchDeltaAlt = ReadAt<float>(table, k_InputPitchDeltaAltOffset);
        frame.Tail6D0 = ReadAt<float>(table, k_InputTailOffset);
        return frame;
    }

    // Reads the fields of the client block the pure client keeps between calls.
    auto ReadClientState(std::span<const std::uint8_t, k_ClientBlockWindow> block) -> ClientState
    {
        ClientState state{};
        state.SelectionIndex = ReadAt<std::uint32_t>(block, k_ClientSelectionIndexOffset);
        state.SelectionKind = ReadAt<std::uint32_t>(block, k_ClientSelectionKindOffset);
        state.LookRateActive = ReadAt<std::uint8_t>(block, k_ClientLookRateActiveOffset);
        state.LookRateYaw = ReadAt<float>(block, k_ClientLookRateYawOffset);
        state.LookRatePitch = ReadAt<float>(block, k_ClientLookRateYawOffset + sizeof(float));
        state.LookRemainderYaw = ReadAt<float>(block, k_ClientLookRemainderYawOffset);
        state.LookRemainderPitch = ReadAt<float>(block, k_ClientLookRemainderYawOffset + sizeof(float));
        state.Yaw = ReadAt<float>(block, k_ClientYawOffset);
        state.Pitch = ReadAt<float>(block, k_ClientYawOffset + sizeof(float));
        state.ZoomLevel = ReadAt<std::int16_t>(block, k_ClientZoomLevelOffset);
        state.AimAssistActive = ReadAt<std::uint8_t>(block, k_ClientAimAssistOffset);
        state.FrameHandle = ReadAt<std::uint32_t>(block, k_ClientFrameHandleOffset);
        state.JumpTimer = ReadAt<float>(block, k_ClientJumpTimerOffset);
        state.JumpPhase = ReadAt<std::int32_t>(block, k_ClientJumpTimerOffset + sizeof(float));
        state.IsCrouchToggled = ReadAt<std::uint8_t>(block, k_ClientCrouchOffset) != 0;
        state.CrouchHeldSeconds = ReadAt<float>(block, k_ClientCrouchHeldOffset);
        state.IsSwitchWeaponDown = ReadAt<std::uint8_t>(block, k_ClientSwitchWeaponOffset) != 0;
        state.IsSwitchWeaponLatched = ReadAt<std::uint8_t>(block, k_ClientSwitchWeaponOffset + 1) != 0;
        state.IsSwitchGrenadeDown = ReadAt<std::uint8_t>(block, k_ClientGrenadeOffset) != 0;
        state.IsButton47Down = ReadAt<std::uint8_t>(block, k_ClientGrenadeOffset + 1) != 0;
        state.IsButton48Down = ReadAt<std::uint8_t>(block, k_ClientGrenadeOffset + 2) != 0;
        state.IsGrenadeLatched = ReadAt<std::uint8_t>(block, k_ClientGrenadeOffset + 3) != 0;
        return state;
    }

    // Compares a step of the pure client with what the engine left in its output and in its client block, bit by bit.
    // param step: Result of the pure client for the entry of the call.
    // param output: The 0x60 bytes of the engine output after the call.
    // param block: The client block after the call.
    // return: What differs. Floats are compared by their bits.
    auto CompareClient(const ClientStep& step, std::span<const std::uint8_t, k_ClientOutputSize> output,
        std::span<const std::uint8_t, k_ClientBlockWindow> block) -> ClientDifferences
    {
        ClientDifferences differences{};
        const auto& out{ step.Output };

        differences.IsMove = !IsSame(out.MoveForward, ReadAt<float>(output, k_OutputMoveOffset))
            || !IsSame(out.MoveSide, ReadAt<float>(output, k_OutputMoveOffset + sizeof(float)));
        differences.IsTriggers = !IsSame(out.FireTrigger, ReadAt<float>(output, k_OutputFireTriggerOffset))
            || !IsSame(out.ZoomTrigger, ReadAt<float>(output, k_OutputZoomTriggerOffset))
            || !IsSame(out.Trigger4, ReadAt<float>(output, k_OutputTrigger4Offset));
        differences.IsLook = !IsSame(out.YawDelta, ReadAt<float>(output, k_OutputLookOffset))
            || !IsSame(out.PitchDelta, ReadAt<float>(output, k_OutputLookOffset + sizeof(float)));
        differences.Buttons = out.Buttons ^ ReadAt<std::uint64_t>(output, k_OutputButtonsOffset);
        differences.Events = out.Events ^ ReadAt<std::uint32_t>(output, k_OutputEventsOffset);
        differences.Lanes = out.Lanes ^ ReadAt<std::uint32_t>(output, k_OutputLanesOffset);
        differences.IsTail = !IsSame(out.Tail30, ReadAt<float>(output, k_OutputTailOffset));

        const ClientState engine{ ReadClientState(block) };
        const ClientState& state{ step.State };
        auto mark = [&](ClientStateField field, bool isDifferent) {
            if (isDifferent) differences.StateFields |= 1u << static_cast<std::uint32_t>(field);
        };

        mark(ClientStateField::Selection, state.SelectionIndex != engine.SelectionIndex);
        mark(ClientStateField::LookRates, state.LookRateActive != engine.LookRateActive || !IsSame(state.LookRateYaw, engine.LookRateYaw)
            || !IsSame(state.LookRatePitch, engine.LookRatePitch) || !IsSame(state.LookRemainderYaw, engine.LookRemainderYaw)
            || !IsSame(state.LookRemainderPitch, engine.LookRemainderPitch));
        mark(ClientStateField::Zoom, state.ZoomLevel != engine.ZoomLevel);
        mark(ClientStateField::AimAssist, state.AimAssistActive != engine.AimAssistActive);
        mark(ClientStateField::Frame, state.FrameHandle != engine.FrameHandle);
        mark(ClientStateField::Jump, !IsSame(state.JumpTimer, engine.JumpTimer) || state.JumpPhase != engine.JumpPhase);
        mark(ClientStateField::Crouch, state.IsCrouchToggled != engine.IsCrouchToggled
            || !IsSame(state.CrouchHeldSeconds, engine.CrouchHeldSeconds));
        mark(ClientStateField::SwitchWeapon, state.IsSwitchWeaponDown != engine.IsSwitchWeaponDown
            || state.IsSwitchWeaponLatched != engine.IsSwitchWeaponLatched);
        mark(ClientStateField::Grenade, state.IsSwitchGrenadeDown != engine.IsSwitchGrenadeDown
            || state.IsButton47Down != engine.IsButton47Down || state.IsButton48Down != engine.IsButton48Down
            || state.IsGrenadeLatched != engine.IsGrenadeLatched);

        return differences;
    }
}