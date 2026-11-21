#include <gtest/gtest.h>

import Control.Client.Type;
import Control.Client.System;
import std;

// Control.Client (VW.9c.1, D356): the client of the engine as a pure function. One test per block of 0x5df80, with the inputs of A17 sec. 9.

namespace
{
    using Control::Client::System::IntegrateAim;
    using Control::Client::System::StepClient;
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

    using Control::Client::Type::k_ButtonCrouch;
    using Control::Client::Type::k_ButtonFire;
    using Control::Client::Type::k_ButtonJump;
    using Control::Client::Type::k_ButtonMelee;
    using Control::Client::Type::k_ButtonThrow;
    using Control::Client::Type::k_ButtonZoom;
    using Control::Client::Type::k_EventReload;
    using Control::Client::Type::k_EventSwitchGrenade;
    using Control::Client::Type::k_EventThrowGrenade;
    using Control::Client::Type::k_EventZoomHeld;
    using Control::Client::Type::k_EventZoomPress;
    using Control::Client::Type::k_AllUnitFacts;
    using Control::Client::Type::k_PitchLimit;

    constexpr float k_Dt{ 1.0f / 60.0f };

    auto Hold(InputFrame& frame, InputEvent event, std::uint16_t heldMs = 1, float value = 1.0f) -> void
    {
        frame.Events[static_cast<std::size_t>(event)] = EventSample{ .IsDown = true, .HeldMs = heldMs, .Value = value };
    }

    auto Bit(std::uint32_t bit) -> std::uint64_t { return 1ull << bit; }

    auto OnFoot() -> UnitContext { return UnitContext{ .HasUnit = true }; }

    auto Seated() -> UnitContext { return UnitContext{ .HasUnit = true, .IsSeated = true }; }

    auto RunClient(const InputFrame& frame, const UnitContext& unit, const ClientSettings& settings = {}, const ClientState& state = {}) -> ClientStep
    {
        return StepClient(frame, state, unit, settings, k_Dt);
    }
}

TEST(ClientTests, TheMoveAxesGoToTheOutput)
{
    InputFrame frame{};
    frame.MoveForward = 1.0f;
    frame.MoveSide = -1.0f;

    const ClientStep step{ RunClient(frame, OnFoot()) };
    EXPECT_EQ(step.Status, ClientStatus::Ran);
    EXPECT_EQ(step.Output.MoveForward, 1.0f);
    EXPECT_EQ(step.Output.MoveSide, -1.0f);
}

TEST(ClientTests, TheWeaponInTheHandsSlowsTheMoveAxes)
{
    InputFrame frame{};
    frame.MoveForward = 1.0f;
    frame.MoveSide = 1.0f;
    UnitContext unit{ OnFoot() };
    unit.MovePenaltyForward = 0.25f;

    const ClientStep step{ RunClient(frame, unit) };
    EXPECT_EQ(step.Output.MoveForward, 0.75f);
    EXPECT_EQ(step.Output.MoveSide, 1.0f);
}

TEST(ClientTests, TheLookDeltaWithoutZoomPassesThrough)
{
    InputFrame frame{};
    frame.YawDelta = 0.12f;
    frame.PitchDelta = -0.06f;
    frame.PitchDeltaAlt = 0.06f;

    const ClientStep step{ RunClient(frame, OnFoot()) };
    EXPECT_NEAR(step.Output.YawDelta, 0.12f, 1e-6f);
    EXPECT_NEAR(step.Output.PitchDelta, -0.06f, 1e-6f);
}

TEST(ClientTests, ZoomWithMagnificationThreeScalesTheLookDelta)
{
    InputFrame frame{};
    frame.YawDelta = 0.12f;
    frame.PitchDelta = 0.12f;
    UnitContext unit{ OnFoot() };
    unit.ZoomMagnification = 3.0f;
    ClientState state{};
    state.ZoomLevel = 0;

    const ClientStep step{ RunClient(frame, unit, ClientSettings{}, state) };
    EXPECT_NEAR(step.Output.YawDelta, 0.04f, 1e-6f);
    EXPECT_NEAR(step.Output.PitchDelta, 0.04f, 1e-6f);
}

TEST(ClientTests, TheAimScaleOfTheUserAppliesInASeat)
{
    InputFrame frame{};
    frame.YawDelta = 0.1f;
    ClientSettings settings{};
    settings.AimScaleSeated = 0.5f;

    EXPECT_NEAR(RunClient(frame, Seated(), settings).Output.YawDelta, 0.05f, 1e-6f);
    EXPECT_NEAR(RunClient(frame, OnFoot(), settings).Output.YawDelta, 0.1f, 1e-6f);
}

TEST(ClientTests, ThePitchStaysAtTheLimitWhenTheDeltaPushesUp)
{
    ClientState state{};
    state.Pitch = k_PitchLimit;

    const ClientState up{ IntegrateAim(state, 0.0f, 0.12f, OnFoot()) };
    EXPECT_EQ(up.Pitch, k_PitchLimit);

    state.Pitch = -k_PitchLimit;
    const ClientState down{ IntegrateAim(state, 0.0f, -0.12f, OnFoot()) };
    EXPECT_EQ(down.Pitch, -k_PitchLimit);
}

TEST(ClientTests, TheYawWrapsIntoOneTurn)
{
    ClientState state{};
    state.Yaw = 6.2f;

    const ClientState next{ IntegrateAim(state, 0.2f, 0.0f, OnFoot()) };
    EXPECT_GE(next.Yaw, 0.0f);
    EXPECT_LT(next.Yaw, 0.2f);
}

TEST(ClientTests, TheDirectButtonsSetTheirBits)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Fire, 1, 0.5f);
    Hold(frame, InputEvent::Jump);
    Hold(frame, InputEvent::Melee);
    Hold(frame, InputEvent::Throw);
    Hold(frame, InputEvent::Reload);
    Hold(frame, InputEvent::ThrowGrenade);

    const ClientStep step{ RunClient(frame, OnFoot()) };
    EXPECT_NE(step.Output.Buttons & k_ButtonFire, 0u);
    EXPECT_NE(step.Output.Buttons & k_ButtonJump, 0u);
    EXPECT_NE(step.Output.Buttons & k_ButtonMelee, 0u);
    EXPECT_NE(step.Output.Buttons & k_ButtonThrow, 0u);
    EXPECT_EQ(step.Output.FireTrigger, 0.5f);
    EXPECT_NE(step.Output.Events & k_EventReload, 0u);
    EXPECT_NE(step.Output.Events & k_EventThrowGrenade, 0u);
}

TEST(ClientTests, JumpInASeatIsTheSeatButtonNotTheJump)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Jump);
    Hold(frame, InputEvent::SeatJump);
    Hold(frame, InputEvent::SeatCrouch);

    const ClientStep step{ RunClient(frame, Seated()) };
    EXPECT_EQ(step.Output.Buttons & k_ButtonJump, 0u);
    EXPECT_NE(step.Output.Buttons & Bit(20), 0u);
    EXPECT_NE(step.Output.Buttons & Bit(2), 0u);
}

TEST(ClientTests, CrouchAsAToggleStaysUntilTheSecondPress)
{
    ClientSettings settings{};
    settings.IsCrouchToggle = true;

    InputFrame pressed{};
    Hold(pressed, InputEvent::Crouch);
    const InputFrame released{};

    ClientStep step{ RunClient(pressed, OnFoot(), settings) };
    EXPECT_NE(step.Output.Buttons & k_ButtonCrouch, 0u);

    step = RunClient(released, OnFoot(), settings, step.State);
    EXPECT_NE(step.Output.Buttons & k_ButtonCrouch, 0u);

    step = RunClient(pressed, OnFoot(), settings, step.State);
    EXPECT_EQ(step.Output.Buttons & k_ButtonCrouch, 0u);
}

TEST(ClientTests, CrouchAsAHoldFollowsTheButton)
{
    InputFrame pressed{};
    Hold(pressed, InputEvent::Crouch);

    ClientStep step{ RunClient(pressed, OnFoot()) };
    EXPECT_NE(step.Output.Buttons & k_ButtonCrouch, 0u);

    step = RunClient(InputFrame{}, OnFoot(), ClientSettings{}, step.State);
    EXPECT_EQ(step.Output.Buttons & k_ButtonCrouch, 0u);
}

TEST(ClientTests, UseHeldLongerThanTheThresholdSetsLane21)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Use, 600);

    const ClientStep step{ RunClient(frame, OnFoot()) };
    EXPECT_NE(step.Output.Lanes & (1u << 20), 0u);
    EXPECT_NE(step.Output.Lanes & (1u << 21), 0u);

    InputFrame tap{};
    Hold(tap, InputEvent::Use, 217);
    const ClientStep tapped{ RunClient(tap, OnFoot()) };
    EXPECT_NE(tapped.Output.Lanes & (1u << 20), 0u);
    EXPECT_EQ(tapped.Output.Lanes & (1u << 21), 0u);
}

TEST(ClientTests, ReloadSetsEventBitEightAndLaneSixteen)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Reload);
    Hold(frame, InputEvent::LaneButton);

    const ClientStep step{ RunClient(frame, OnFoot()) };
    EXPECT_NE(step.Output.Events & k_EventReload, 0u);
    EXPECT_NE(step.Output.Lanes & (1u << 16), 0u);
}

TEST(ClientTests, SwitchWeaponAsTapMakesTheEventAndAHoldMakesTheLane)
{
    InputFrame down{};
    Hold(down, InputEvent::SwitchWeapon, 100);

    ClientStep step{ RunClient(down, OnFoot()) };
    EXPECT_NE(step.Output.Lanes & (1u << 8), 0u);
    EXPECT_EQ(step.Output.Events & 1u, 0u);

    step = RunClient(InputFrame{}, OnFoot(), ClientSettings{}, step.State);
    EXPECT_NE(step.Output.Events & 1u, 0u);
}

TEST(ClientTests, SwitchGrenadeWithoutGrenadesStillAsksWhenReleased)
{
    InputFrame down{};
    Hold(down, InputEvent::SwitchGrenade);

    ClientStep step{ RunClient(down, OnFoot()) };
    EXPECT_EQ(step.Output.Events & k_EventSwitchGrenade, 0u);

    step = RunClient(InputFrame{}, OnFoot(), ClientSettings{}, step.State);
    EXPECT_NE(step.Output.Events & k_EventSwitchGrenade, 0u);
}

TEST(ClientTests, ZoomPressMakesTheZoomEventsAndAWeaponWithoutZoomCancelsItWithALargeLook)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Zoom);
    Hold(frame, InputEvent::ZoomValue);

    const ClientStep step{ RunClient(frame, OnFoot()) };
    EXPECT_NE(step.Output.Events & k_EventZoomPress, 0u);
    EXPECT_NE(step.Output.Events & k_EventZoomHeld, 0u);
    EXPECT_NE(step.Output.Buttons & k_ButtonZoom, 0u);

    UnitContext unit{ OnFoot() };
    unit.HasZoomItem = true;
    unit.IsZoomItemWithoutLevels = true;
    frame.YawDelta = 1.0f;
    const ClientStep cancelled{ RunClient(frame, unit) };
    EXPECT_EQ(cancelled.Output.Events & k_EventZoomPress, 0u);
    EXPECT_NE(cancelled.Output.Events & k_EventZoomHeld, 0u);
}

TEST(ClientTests, TheRightClickInADriverSeatSetsBitsFifteenEighteenThirtyOneAndThirtyFive)
{
    InputFrame frame{};
    Hold(frame, InputEvent::ZoomValue);

    const ClientStep step{ RunClient(frame, Seated()) };
    const std::uint64_t expected{ Bit(15) | Bit(18) | Bit(31) | Bit(35) };
    EXPECT_EQ(step.Output.Buttons & expected, expected);

    const ClientStep onFoot{ RunClient(frame, OnFoot()) };
    EXPECT_EQ(onFoot.Output.Buttons & (Bit(15) | Bit(31) | Bit(35)), 0u);
    EXPECT_NE(onFoot.Output.Buttons & Bit(18), 0u);
}

TEST(ClientTests, TheAbilityOfTheHoldKindSetsLaneThirtyOne)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Ability);
    UnitContext unit{ OnFoot() };
    unit.Ability.IsPresent = true;

    const ClientStep step{ RunClient(frame, unit) };
    EXPECT_NE(step.Output.Lanes & (1u << 31), 0u);
    EXPECT_NE(step.Output.Lanes & (1u << 12), 0u);
}

TEST(ClientTests, TheAbilityButtonKindGoesThroughTheEngineOnlyWithTheJumpFlag)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Ability);
    UnitContext unit{ OnFoot() };
    unit.Ability.IsPresent = true;
    unit.Ability.Kind = 1;
    unit.Ability.NeedsEngine = true;

    const ClientStep plain{ RunClient(frame, unit) };
    EXPECT_EQ(plain.Output.Unsupported & static_cast<std::uint32_t>(Unsupported::AbilityButton), 0u);
    EXPECT_NE(plain.Output.Buttons & Bit(8), 0u);

    ClientSettings settings{};
    settings.IsJumpFlag = true;
    const ClientStep flagged{ RunClient(frame, unit, settings) };
    EXPECT_NE(flagged.Output.Unsupported & static_cast<std::uint32_t>(Unsupported::AbilityButton), 0u);
}

TEST(ClientTests, ACarriedDropSetsTheAbilityButtonAndResetsTheZoom)
{
    UnitContext unit{ OnFoot() };
    unit.IsCarriedDrop = true;
    ClientState state{};
    state.ZoomLevel = 1;
    state.IsCrouchToggled = true;

    const ClientStep step{ RunClient(InputFrame{}, unit, ClientSettings{}, state) };
    EXPECT_NE(step.Output.Buttons & Bit(16), 0u);
    EXPECT_EQ(step.State.ZoomLevel, -1);
    EXPECT_FALSE(step.State.IsCrouchToggled);
}

TEST(ClientTests, TheJumpTimerCountsDownAndThePhaseFollowsTheButton)
{
    ClientSettings settings{};
    settings.IsJumpFlag = true;
    settings.JumpTimerSeconds = 0.5f;

    InputFrame pressed{};
    Hold(pressed, InputEvent::Jump);

    const ClientStep first{ RunClient(pressed, OnFoot(), settings) };
    EXPECT_EQ(first.State.JumpPhase, 2);
    EXPECT_EQ(first.State.JumpTimer, 0.5f);

    const ClientStep second{ RunClient(pressed, OnFoot(), settings, first.State) };
    EXPECT_LT(second.State.JumpTimer, 0.5f);
}

TEST(ClientTests, AModeOtherThanKeyboardAndMouseDoesNotRun)
{
    InputFrame frame{};
    frame.Mode = 0;
    frame.MoveForward = 1.0f;

    const ClientStep step{ RunClient(frame, OnFoot()) };
    EXPECT_EQ(step.Status, ClientStatus::WrongMode);
    EXPECT_EQ(step.Output.MoveForward, 0.0f);

    frame.Mode = 1;
    frame.IsMouse = false;
    EXPECT_EQ(RunClient(frame, OnFoot()).Status, ClientStatus::NotMouse);
}

TEST(ClientTests, ABranchThatNeedsTheEngineIsReportedNotGuessed)
{
    UnitContext unit{ OnFoot() };
    unit.IsLookOffset = true;
    unit.SelectionEntries = 2;

    const ClientStep step{ RunClient(InputFrame{}, unit) };
    EXPECT_NE(step.Output.Unsupported & static_cast<std::uint32_t>(Unsupported::LookOffset), 0u);
    EXPECT_NE(step.Output.Unsupported & static_cast<std::uint32_t>(Unsupported::SelectionList), 0u);
}

TEST(ClientTests, TheSameInputsGiveTheSameStep)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Fire);
    frame.YawDelta = 0.07f;
    frame.MoveForward = 0.5f;

    const ClientStep a{ RunClient(frame, OnFoot()) };
    const ClientStep b{ RunClient(frame, OnFoot()) };
    EXPECT_EQ(std::memcmp(&a.Output, &b.Output, sizeof(a.Output)), 0);
}

TEST(ClientTests, AContextThatKnowsEveryFactReadsNoUnknownFact)
{
    InputFrame frame{};
    Hold(frame, InputEvent::Fire);

    EXPECT_EQ(RunClient(frame, OnFoot()).Output.UnknownRead, 0u);
    EXPECT_EQ(RunClient(frame, Seated()).Output.UnknownRead, 0u);
}

TEST(ClientTests, OnlyTheUnknownFactsTheStepReadsAreRecorded)
{
    UnitContext unit{ .HasUnit = true };
    unit.Known = static_cast<std::uint32_t>(UnitFact::HasUnit) | static_cast<std::uint32_t>(UnitFact::IsSeated);

    const std::uint32_t mask{ RunClient(InputFrame{}, unit).Output.UnknownRead };

    EXPECT_NE(mask & static_cast<std::uint32_t>(UnitFact::LookRated), 0u);
    EXPECT_NE(mask & static_cast<std::uint32_t>(UnitFact::MovePenalty), 0u);
    EXPECT_NE(mask & static_cast<std::uint32_t>(UnitFact::JumpAssist), 0u);
    EXPECT_EQ(mask & static_cast<std::uint32_t>(UnitFact::HasUnit), 0u);
    EXPECT_EQ(mask & static_cast<std::uint32_t>(UnitFact::IsSeated), 0u);
    // Not zoomed: the magnification is never read.
    EXPECT_EQ(mask & static_cast<std::uint32_t>(UnitFact::ZoomMagnification), 0u);
}

TEST(ClientTests, TheMagnificationIsReadOnlyWhileZoomed)
{
    UnitContext unit{ .HasUnit = true };
    unit.Known = static_cast<std::uint32_t>(UnitFact::HasUnit) | static_cast<std::uint32_t>(UnitFact::IsSeated);
    ClientState state{};
    state.ZoomLevel = 0;

    EXPECT_NE(RunClient(InputFrame{}, unit, ClientSettings{}, state).Output.UnknownRead & static_cast<std::uint32_t>(UnitFact::ZoomMagnification), 0u);
}

TEST(ClientTests, TheFactsOfABranchNotTakenAreNotRecorded)
{
    UnitContext unit{};
    unit.Known = static_cast<std::uint32_t>(UnitFact::HasUnit) | static_cast<std::uint32_t>(UnitFact::IsSeated);

    const std::uint32_t mask{ RunClient(InputFrame{}, unit).Output.UnknownRead };

    // Without a unit the penalty, the damping, the biped and the weapon flags are never read.
    EXPECT_EQ(mask & static_cast<std::uint32_t>(UnitFact::MovePenalty), 0u);
    EXPECT_EQ(mask & static_cast<std::uint32_t>(UnitFact::AimDamp), 0u);
    EXPECT_EQ(mask & static_cast<std::uint32_t>(UnitFact::Biped), 0u);
    EXPECT_EQ(mask & static_cast<std::uint32_t>(UnitFact::WeaponTriggerFlag), 0u);
    EXPECT_NE(mask & static_cast<std::uint32_t>(UnitFact::LookRated), 0u);
}

TEST(ClientTests, AFactTheContextKnowsIsNeverRecordedEvenWhenRead)
{
    UnitContext unit{ .HasUnit = true };
    unit.Known = k_AllUnitFacts & ~static_cast<std::uint32_t>(UnitFact::LookRated);

    EXPECT_EQ(RunClient(InputFrame{}, unit).Output.UnknownRead, static_cast<std::uint32_t>(UnitFact::LookRated));
}
