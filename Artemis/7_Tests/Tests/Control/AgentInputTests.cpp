#include <gtest/gtest.h>

import Control.Client.Type;
import Control.Client.System;
import std;

// Control.Client agent input (VW.9c.3 B, D362-D364): the synthesis of one frame per tick from an AgentInput and the reduction of the
// frames of a window back into an AgentInput. Expected events are written here from A17 sec. 9, not read from the map under test.

namespace
{
    using Control::Client::System::AddAgentFrame;
    using Control::Client::System::CloseAgentInput;
    using Control::Client::System::FindAgentControl;
    using Control::Client::System::AreTickRangesOverlapping;
    using Control::Client::System::ParseAgentInputLine;
    using Control::Client::System::ParseAgentInputWords;
    using Control::Client::System::IntegrateAim;
    using Control::Client::System::MakeAgentFrame;
    using Control::Client::System::SanitizeAgentInput;
    using Control::Client::System::StepClient;
    using Control::Client::Type::AgentControl;
    using Control::Client::Type::AgentInput;
    using Control::Client::Type::AgentInputReducer;
    using Control::Client::Type::AgentInputLine;
    using Control::Client::Type::AgentInputState;
    using Control::Client::Type::ClientSettings;
    using Control::Client::Type::ClientState;
    using Control::Client::Type::ClientStep;
    using Control::Client::Type::EventSample;
    using Control::Client::Type::InputEvent;
    using Control::Client::Type::InputFrame;
    using Control::Client::Type::UnitContext;

    using Control::Client::Type::k_AgentControlCount;
    using Control::Client::Type::k_AgentControlMask;
    using Control::Client::Type::k_AgentControlNames;
    using Control::Client::Type::k_EventCount;
    using Control::Client::Type::k_PitchLimit;

    constexpr float k_Dt{ 1.0f / 60.0f };

    // The events each control raises with the default binds (A17 sec. 9): the primary first, the rest after it, by raw index.
    struct Measured
    {
        std::size_t Primary{ 0 };
        std::vector<std::size_t> Others{};
        std::vector<std::size_t> WithValue{};
    };

    const std::array<Measured, k_AgentControlCount> k_Measured{ {
        { 0x00, { 0x0D, 0x35, 0x41 }, {} },
        { 0x08, { 0x0C, 0x27 }, {} },
        { 0x07, { 0x17, 0x22 }, { 0x07 } },
        { 0x09, { 0x0A, 0x13, 0x28, 0x31, 0x42 }, { 0x0A } },
        { 0x3C, { 0x3F }, {} },
        { 0x3D, { 0x24, 0x2F }, {} },
        { 0x04, {}, {} },
        { 0x05, { 0x26, 0x40 }, {} },
        { 0x06, { 0x1F }, {} },
        { 0x03, {}, {} },
        { 0x02, { 0x20 }, {} },
        { 0x01, { 0x23 }, {} },
        { 0x1E, { 0x3E, 0x44 }, {} },
    } };

    auto Bit(std::uint32_t control) -> std::uint32_t { return 1u << control; }

    auto OnFoot() -> UnitContext { return UnitContext{ .HasUnit = true }; }

    auto PrimaryOf(std::uint32_t control) -> std::size_t { return k_Measured[control].Primary; }

    auto Window(const AgentInput& input, std::int32_t length, AgentInputState& state, float turnLimit = 0.0f) -> std::vector<InputFrame>
    {
        std::vector<InputFrame> frames{};
        for (std::int32_t tick{ 0 }; tick < length; ++tick)
        {
            frames.push_back(MakeAgentFrame(input, tick, length, ClientSettings{}, turnLimit, state));
        }
        return frames;
    }

    auto HeldMsOf(int ticks) -> int { return static_cast<int>(std::lround(ticks * 1000.0 / 60.0)); }

    // Runs the client over the frames of a window and reduces them, as the possessed client does.
    auto RoundTrip(const std::vector<InputFrame>& frames, AgentInputReducer& reducer, const UnitContext& unit = OnFoot()) -> AgentInput
    {
        ClientState state{};
        for (const InputFrame& frame : frames)
        {
            const ClientStep step{ StepClient(frame, state, unit, ClientSettings{}, k_Dt) };
            state = step.State;
            AddAgentFrame(reducer, frame, step.Output.LookYawRequested, step.Output.LookPitchRequested);
        }
        return CloseAgentInput(reducer);
    }

    // The four states of D362 and the repress, as (level, edge, was down at the window start) and the expected down counts of 4 ticks.
    struct Case
    {
        const char* Name{ "" };
        bool IsLevel{ false };
        bool IsEdge{ false };
        bool WasDown{ false };
        std::array<int, 4> Counts{};
    };

    constexpr std::array<Case, 8> k_Cases{ {
        { "nothing", false, false, false, { 0, 0, 0, 0 } },
        { "tap", false, true, false, { 1, 0, 0, 0 } },
        { "press and hold", true, true, false, { 1, 2, 3, 4 } },
        { "hold without having been down", true, false, false, { 1, 2, 3, 4 } },
        { "hold on", true, false, true, { 5, 6, 7, 8 } },
        { "release", false, false, true, { 0, 0, 0, 0 } },
        { "press again", true, true, true, { 0, 1, 2, 3 } },
        { "tap again", false, true, true, { 0, 1, 0, 0 } },
    } };

    // Puts a control down for one window of 4 ticks so the next one starts with it down (counts 1 to 4).
    auto Prime(std::uint32_t control, AgentInputState& state) -> void
    {
        Window(AgentInput{ .Level = Bit(control), .Edge = Bit(control) }, 4, state);
    }
}

TEST(AgentInputTests, TheSyntheticFrameOfEachControlHasTheMeasuredEventsAndNothingElse)
{
    for (std::uint32_t control{ 0 }; control < k_AgentControlCount; ++control)
    {
        AgentInputState state{};
        const InputFrame frame{ MakeAgentFrame(AgentInput{ .Level = Bit(control), .Edge = Bit(control) }, 0, 4, ClientSettings{}, 0.0f, state) };

        for (std::size_t index{ 0 }; index < k_EventCount; ++index)
        {
            const Measured& measured{ k_Measured[control] };
            const bool isExpected{ index == measured.Primary || std::ranges::find(measured.Others, index) != measured.Others.end() };
            const bool isValued{ std::ranges::find(measured.WithValue, index) != measured.WithValue.end() };
            const EventSample& sample{ frame.Events[index] };
            EXPECT_EQ(sample.IsDown, isExpected) << "control " << control << " event " << index;
            EXPECT_EQ(sample.DownCount, isExpected ? 1 : 0) << "control " << control << " event " << index;
            EXPECT_EQ(sample.HeldMs, isExpected ? HeldMsOf(1) : 0) << "control " << control << " event " << index;
            EXPECT_EQ(sample.Value, isValued ? 1.0f : 0.0f) << "control " << control << " event " << index;
        }

        EXPECT_EQ(frame.MoveForward, 0.0f);
        EXPECT_EQ(frame.MoveSide, 0.0f);
        EXPECT_EQ(frame.YawDelta, 0.0f);
        EXPECT_EQ(frame.PitchDelta, 0.0f);
    }
}

TEST(AgentInputTests, EachStateOfEachControlGivesTheCountsAndTheMillisecondsTickByTick)
{
    for (const std::int32_t length : { 4, 2 })
    {
        for (std::uint32_t control{ 0 }; control < k_AgentControlCount; ++control)
        {
            for (const Case& current : k_Cases)
            {
                AgentInputState state{};
                if (current.WasDown) Prime(control, state);

                const AgentInput input{ .Level = current.IsLevel ? Bit(control) : 0, .Edge = current.IsEdge ? Bit(control) : 0 };
                const std::vector<InputFrame> frames{ Window(input, length, state) };
                for (std::int32_t tick{ 0 }; tick < length; ++tick)
                {
                    const EventSample& sample{ frames[static_cast<std::size_t>(tick)].Events[PrimaryOf(control)] };
                    const int count{ current.Counts[static_cast<std::size_t>(tick)] };
                    const std::string where{ std::string{ current.Name } + ", control " + std::to_string(control) + ", d " +
                        std::to_string(length) + ", tick " + std::to_string(tick) };
                    EXPECT_EQ(sample.IsDown, count != 0) << where;
                    EXPECT_EQ(sample.DownCount, count) << where;
                    EXPECT_EQ(sample.HeldMs, count == 0 ? 0 : HeldMsOf(count)) << where;
                }
            }
        }
    }
}

TEST(AgentInputTests, NothingThenHoldThenHoldOnThenNothingThenTapGivesTheExpectedSequence)
{
    constexpr std::uint32_t control{ static_cast<std::uint32_t>(AgentControl::Use) };
    AgentInputState state{};
    const std::array<AgentInput, 5> windows{ {
        { .Level = 0, .Edge = 0 },
        { .Level = Bit(control), .Edge = Bit(control) },
        { .Level = Bit(control), .Edge = 0 },
        { .Level = 0, .Edge = 0 },
        { .Level = 0, .Edge = Bit(control) },
    } };
    const std::array<std::array<int, 4>, 5> expected{ {
        { 0, 0, 0, 0 }, { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 0, 0, 0, 0 }, { 1, 0, 0, 0 },
    } };

    for (std::size_t window{ 0 }; window < windows.size(); ++window)
    {
        const std::vector<InputFrame> frames{ Window(windows[window], 4, state) };
        for (std::size_t tick{ 0 }; tick < 4; ++tick)
        {
            EXPECT_EQ(frames[tick].Events[PrimaryOf(control)].DownCount, expected[window][tick]) << "window " << window << " tick " << tick;
        }
    }
}

TEST(AgentInputTests, TheDownCountStopsAt255)
{
    AgentInputState state{};
    const AgentInput input{ .Level = Bit(0), .Edge = Bit(0) };
    InputFrame frame{};
    for (std::int32_t tick{ 0 }; tick < 300; ++tick) frame = MakeAgentFrame(input, tick, 300, ClientSettings{}, 0.0f, state);
    EXPECT_EQ(frame.Events[PrimaryOf(0)].DownCount, 255);
    EXPECT_EQ(frame.Events[PrimaryOf(0)].HeldMs, HeldMsOf(300));
}

TEST(AgentInputTests, AUseHeldForHalfASecondSetsTheHoldLaneOfTheClient)
{
    constexpr std::uint32_t use{ static_cast<std::uint32_t>(AgentControl::Use) };
    AgentInputState state{};
    const std::vector<InputFrame> frames{ Window(AgentInput{ .Level = Bit(use), .Edge = Bit(use) }, 30, state) };
    const ClientSettings settings{};
    const ClientStep early{ StepClient(frames[28], ClientState{}, OnFoot(), settings, k_Dt) };
    const ClientStep late{ StepClient(frames[29], ClientState{}, OnFoot(), settings, k_Dt) };
    EXPECT_EQ(early.Output.Lanes & (1u << 21), 0u);
    EXPECT_NE(late.Output.Lanes & (1u << 21), 0u);
    EXPECT_NE(late.Output.Lanes & (1u << 20), 0u);
}

TEST(AgentInputTests, TheMoveAxesAreTheSameInEveryTickAndLeftIsPositiveMoveSide)
{
    AgentInputState state{};
    const std::vector<InputFrame> frames{ Window(AgentInput{ .MoveForward = -0.5f, .MoveLeft = 1.0f }, 4, state) };
    for (const InputFrame& frame : frames)
    {
        EXPECT_EQ(frame.MoveForward, -0.5f);
        EXPECT_EQ(frame.MoveSide, 1.0f);
        const ClientStep step{ StepClient(frame, ClientState{}, OnFoot(), ClientSettings{}, k_Dt) };
        EXPECT_EQ(step.Output.MoveForward, -0.5f);
        EXPECT_EQ(step.Output.MoveSide, 1.0f);
    }
}

TEST(AgentInputTests, TheClientWithoutZoomAndWithoutLimitTurnsWhatWasAsked)
{
    for (const std::int32_t length : { 4, 2 })
    {
        AgentInputState state{};
        const std::vector<InputFrame> frames{ Window(AgentInput{ .YawDelta = -0.37f, .PitchDelta = 0.21f }, length, state) };
        float yaw{ 0.0f };
        float pitch{ 0.0f };
        for (const InputFrame& frame : frames)
        {
            const ClientStep step{ StepClient(frame, ClientState{}, OnFoot(), ClientSettings{}, k_Dt) };
            yaw += step.Output.YawDelta;
            pitch += step.Output.PitchDelta;
        }

        EXPECT_NEAR(yaw, -0.37f, 1e-5f);
        EXPECT_NEAR(pitch, 0.21f, 1e-5f);
    }
}

TEST(AgentInputTests, ATurnLimitCutsEachTickAndWhatItCutsIsLost)
{
    AgentInputState state{};
    const std::vector<InputFrame> frames{ Window(AgentInput{ .YawDelta = 0.4f, .PitchDelta = -0.4f }, 4, state, 0.03f) };
    float yaw{ 0.0f };
    float pitch{ 0.0f };
    for (const InputFrame& frame : frames)
    {
        const ClientStep step{ StepClient(frame, ClientState{}, OnFoot(), ClientSettings{}, k_Dt) };
        EXPECT_NEAR(step.Output.YawDelta, 0.03f, 1e-6f);
        yaw += step.Output.YawDelta;
        pitch += step.Output.PitchDelta;
    }

    EXPECT_NEAR(yaw, 4 * 0.03f, 1e-5f);
    EXPECT_NEAR(pitch, -4 * 0.03f, 1e-5f);

    const std::vector<InputFrame> following{ Window(AgentInput{}, 4, state, 0.03f) };
    for (const InputFrame& frame : following) EXPECT_EQ(frame.YawDelta, 0.0f);
}

TEST(AgentInputTests, ThePitchNeverGoesPastTheLimitOfTheEngine)
{
    AgentInputState state{};
    const std::vector<InputFrame> frames{ Window(AgentInput{ .PitchDelta = 2.0f }, 4, state) };
    ClientState client{};
    client.Pitch = 1.4f;
    for (const InputFrame& frame : frames)
    {
        const ClientStep step{ StepClient(frame, client, OnFoot(), ClientSettings{}, k_Dt) };
        client = IntegrateAim(step.State, step.Output.YawDelta, step.Output.PitchDelta, OnFoot());
        EXPECT_LE(client.Pitch, k_PitchLimit);
    }

    EXPECT_EQ(client.Pitch, k_PitchLimit);
}

TEST(AgentInputTests, TheRequestedLookIsTheAskedOneWhileTheOutputGetsTheZoomFactor)
{
    AgentInputState state{};
    const InputFrame frame{ MakeAgentFrame(AgentInput{ .YawDelta = 0.4f, .PitchDelta = 0.4f }, 0, 4, ClientSettings{}, 0.0f, state) };
    UnitContext unit{ OnFoot() };
    unit.ZoomMagnification = 4.0f;
    ClientState client{};
    client.ZoomLevel = 0;

    const ClientStep step{ StepClient(frame, client, unit, ClientSettings{}, k_Dt) };
    EXPECT_NEAR(step.Output.YawDelta, 0.025f, 1e-6f);
    EXPECT_NEAR(step.Output.LookYawRequested, 0.1f, 1e-6f);
    EXPECT_NEAR(step.Output.LookPitchRequested, 0.1f, 1e-6f);
}

TEST(AgentInputTests, TheRoundTripWithAimDampAndZoomIsExactAndTheTurnIsTheAskedOneScaledByDampAndZoom)
{
    AgentInputState state{};
    const std::vector<InputFrame> frames{ Window(AgentInput{ .YawDelta = 0.4f, .PitchDelta = -0.2f }, 4, state) };
    UnitContext unit{ OnFoot() };
    unit.AimDampTag = 0.5f;
    unit.AimDampUnit = 0.5f;
    unit.ZoomMagnification = 4.0f;

    ClientState client{};
    client.ZoomLevel = 0;
    AgentInputReducer reducer{};
    float yaw{ 0.0f };
    float pitch{ 0.0f };
    for (const InputFrame& frame : frames)
    {
        const ClientStep step{ StepClient(frame, client, unit, ClientSettings{}, k_Dt) };
        client = step.State;
        yaw += step.Output.YawDelta;
        pitch += step.Output.PitchDelta;
        AddAgentFrame(reducer, frame, step.Output.LookYawRequested, step.Output.LookPitchRequested);
    }

    const float scale{ 0.75f / 4.0f };
    EXPECT_NEAR(yaw, 0.4f * scale, 1e-5f);
    EXPECT_NEAR(pitch, -0.2f * scale, 1e-5f);
    const AgentInput back{ CloseAgentInput(reducer) };
    EXPECT_NEAR(back.YawDelta, 0.4f, 1e-6f);
    EXPECT_NEAR(back.PitchDelta, -0.2f, 1e-6f);
}

TEST(AgentInputTests, TheRoundTripGivesTheSameAgentInputForEveryControlAndState)
{
    struct Expectation
    {
        bool IsLevel{ false };
        bool IsEdge{ false };
        bool WasDown{ false };
    };
    constexpr std::array<Expectation, 7> states{ { { false, false, false }, { false, true, false }, { true, true, false },
        { true, false, true }, { false, false, true }, { true, true, true }, { false, true, true } } };

    for (const std::int32_t length : { 4, 3 })
    {
        for (std::uint32_t control{ 0 }; control < k_AgentControlCount; ++control)
        {
            for (const Expectation& wanted : states)
            {
                AgentInputState state{};
                if (wanted.WasDown) Prime(control, state);

                AgentInput input{ .MoveForward = 0.5f, .MoveLeft = -1.0f, .YawDelta = 0.2f, .PitchDelta = -0.1f };
                input.Level = wanted.IsLevel ? Bit(control) : 0;
                input.Edge = wanted.IsEdge ? Bit(control) : 0;

                AgentInputReducer reducer{};
                const AgentInput back{ RoundTrip(Window(input, length, state), reducer) };
                const std::string where{ "control " + std::to_string(control) + " level " + std::to_string(wanted.IsLevel) + " edge " +
                    std::to_string(wanted.IsEdge) + " was down " + std::to_string(wanted.WasDown) + " d " + std::to_string(length) };
                EXPECT_EQ(back.Level, input.Level) << where;
                EXPECT_EQ(back.Edge, input.Edge) << where;
                EXPECT_EQ(back.MoveForward, 0.5f) << where;
                EXPECT_EQ(back.MoveLeft, -1.0f) << where;
                EXPECT_NEAR(back.YawDelta, 0.2f, 1e-5f) << where;
                EXPECT_NEAR(back.PitchDelta, -0.1f, 1e-5f) << where;
            }
        }
    }
}

TEST(AgentInputTests, TheRoundTripOfAllTheControlsAtOnceKeepsEveryBit)
{
    AgentInputState state{};
    const AgentInput input{ .Level = k_AgentControlMask, .Edge = k_AgentControlMask & 0x155Fu };
    AgentInputReducer reducer{};
    const AgentInput back{ RoundTrip(Window(input, 4, state), reducer) };
    EXPECT_EQ(back.Level, input.Level);
    EXPECT_EQ(back.Edge, input.Level);
}

TEST(AgentInputTests, AHoldWithoutHavingBeenDownComesBackAsAPress)
{
    AgentInputState state{};
    AgentInputReducer reducer{};
    const AgentInput back{ RoundTrip(Window(AgentInput{ .Level = Bit(2), .Edge = 0 }, 4, state), reducer) };
    EXPECT_EQ(back.Level, Bit(2));
    EXPECT_EQ(back.Edge, Bit(2));
}

TEST(AgentInputTests, TheReducerTakesTheLastFrameForLevelAndAxesAndAnyFrameForTheEdge)
{
    AgentInputReducer reducer{};
    InputFrame first{};
    first.Events[PrimaryOf(7)] = EventSample{ .IsDown = true, .DownCount = 1, .HeldMs = 17 };
    first.MoveForward = 1.0f;
    InputFrame last{};
    last.Events[PrimaryOf(0)] = EventSample{ .IsDown = true, .DownCount = 9, .HeldMs = 150 };
    last.MoveForward = -1.0f;
    last.MoveSide = 0.25f;

    AddAgentFrame(reducer, first, 0.1f, -0.2f);
    AddAgentFrame(reducer, last, 0.3f, -0.1f);
    const AgentInput result{ CloseAgentInput(reducer) };
    EXPECT_EQ(result.Level, Bit(0));
    EXPECT_EQ(result.Edge, Bit(7));
    EXPECT_EQ(result.MoveForward, -1.0f);
    EXPECT_EQ(result.MoveLeft, 0.25f);
    EXPECT_NEAR(result.YawDelta, 0.4f, 1e-6f);
    EXPECT_NEAR(result.PitchDelta, -0.3f, 1e-6f);
}

TEST(AgentInputTests, AWindowWithoutFramesIsNeutralWithTheLevelOfTheLastFrameSeen)
{
    AgentInputReducer reducer{};
    InputFrame frame{};
    frame.Events[PrimaryOf(3)] = EventSample{ .IsDown = true, .DownCount = 4, .HeldMs = 60 };
    frame.MoveForward = 1.0f;
    AddAgentFrame(reducer, frame, 0.5f, 0.5f);
    const AgentInput first{ CloseAgentInput(reducer) };
    EXPECT_EQ(first.Level, Bit(3));
    EXPECT_EQ(first.Edge, 0u);

    const AgentInput empty{ CloseAgentInput(reducer) };
    EXPECT_EQ(empty.Level, Bit(3));
    EXPECT_EQ(empty.Edge, 0u);
    EXPECT_EQ(empty.MoveForward, 0.0f);
    EXPECT_EQ(empty.YawDelta, 0.0f);
    EXPECT_EQ(empty.PitchDelta, 0.0f);

    AddAgentFrame(reducer, InputFrame{}, 0.0f, 0.0f);
    EXPECT_EQ(CloseAgentInput(reducer).Level, 0u);
}

TEST(AgentInputTests, TheClientOnFootGivesTheBitsAndLanesOfEachKeyWrittenFromTheMeasurements)
{
    // A17 sec. 9.2 (on foot) and sec. 6 and 8, first frame of the key: buttons, event bits, lanes and triggers the client leaves.
    struct Expected
    {
        std::uint64_t Buttons{ 0 };
        std::uint32_t Events{ 0 };
        std::uint32_t Lanes{ 0 };
        float FireTrigger{ 0.0f };
        float ZoomTrigger{ 0.0f };
    };
    const std::array<Expected, k_AgentControlCount> expected{ {
        { 1ull << 1, 0, 0, 0.0f, 0.0f },
        { 1ull << 0, 0, 0, 0.0f, 0.0f },
        { 1ull << 30, 1u << 6, 0, 1.0f, 0.0f },
        { 1ull << 18, (1u << 2) | (1u << 3), 0, 0.0f, 1.0f },
        { 0, 1u << 4, 0, 0.0f, 0.0f },
        { 0, 1u << 5, 0, 0.0f, 0.0f },
        { 1ull << 11, 0, 0, 0.0f, 0.0f },
        { 0, 0, (1u << 12) | (1u << 31), 0.0f, 0.0f },
        { 1ull << 40, 1u << 9, 0, 0.0f, 0.0f },
        { 0, 0, 1u << 20, 0.0f, 0.0f },
        { 0, 0, 1u << 8, 0.0f, 0.0f },
        { 0, 0, 0, 0.0f, 0.0f },
        { 0, 1u << 8, 1u << 16, 0.0f, 0.0f },
    } };

    UnitContext unit{ OnFoot() };
    unit.Ability.IsPresent = true;
    unit.Ability.Kind = 0;
    for (std::uint32_t control{ 0 }; control < k_AgentControlCount; ++control)
    {
        AgentInputState state{};
        const InputFrame frame{ MakeAgentFrame(AgentInput{ .Level = Bit(control), .Edge = Bit(control) }, 0, 4, ClientSettings{}, 0.0f, state) };
        const ClientStep step{ StepClient(frame, ClientState{}, unit, ClientSettings{}, k_Dt) };
        EXPECT_EQ(step.Output.Buttons, expected[control].Buttons) << "control " << control;
        EXPECT_EQ(step.Output.Events, expected[control].Events) << "control " << control;
        EXPECT_EQ(step.Output.Lanes, expected[control].Lanes) << "control " << control;
        EXPECT_EQ(step.Output.FireTrigger, expected[control].FireTrigger) << "control " << control;
        EXPECT_EQ(step.Output.ZoomTrigger, expected[control].ZoomTrigger) << "control " << control;
        EXPECT_EQ(step.Output.Unsupported, 0u) << "control " << control;
    }
}

TEST(AgentInputTests, SanitizingFixesNaNInfinityRangeAndHighBits)
{
    const float nan{ std::numeric_limits<float>::quiet_NaN() };
    const float inf{ std::numeric_limits<float>::infinity() };

    const AgentInput bad{ SanitizeAgentInput(AgentInput{ .MoveForward = nan, .MoveLeft = inf, .YawDelta = -inf, .PitchDelta = nan,
        .Level = 0xFFFFFFFFu, .Edge = 0xFFFFE001u }) };
    EXPECT_EQ(bad.MoveForward, 0.0f);
    EXPECT_EQ(bad.MoveLeft, 0.0f);
    EXPECT_EQ(bad.YawDelta, 0.0f);
    EXPECT_EQ(bad.PitchDelta, 0.0f);
    EXPECT_EQ(bad.Level, k_AgentControlMask);
    EXPECT_EQ(bad.Edge, 1u);

    const AgentInput range{ SanitizeAgentInput(AgentInput{ .MoveForward = 3.0f, .MoveLeft = -7.5f, .YawDelta = 0.4f }) };
    EXPECT_EQ(range.MoveForward, 1.0f);
    EXPECT_EQ(range.MoveLeft, -1.0f);
    EXPECT_EQ(range.YawDelta, 0.4f);

    AgentInputState state{};
    const InputFrame frame{ MakeAgentFrame(AgentInput{ .MoveForward = nan, .YawDelta = nan, .Level = 0x8000u, .Edge = 0x8000u }, 0, 4,
        ClientSettings{}, 0.0f, state) };
    EXPECT_EQ(frame.MoveForward, 0.0f);
    EXPECT_EQ(frame.YawDelta, 0.0f);
    for (const EventSample& sample : frame.Events) EXPECT_FALSE(sample.IsDown);
}

// The text form of an AgentInput (VW.9c.3 C): the names of the controls and the words the test bench reads.

namespace
{
    auto Parse(const std::vector<std::string>& words, AgentInput& input, float& turn) -> std::string
    {
        return ParseAgentInputWords(std::span<const std::string>{ words }, input, turn);
    }
}

TEST(AgentTextTests, EveryControlNameFindsItsControlAndNothingElseDoes)
{
    for (std::uint32_t control{ 0 }; control < k_AgentControlCount; ++control)
    {
        const auto found{ FindAgentControl(k_AgentControlNames[control]) };
        ASSERT_TRUE(found.has_value()) << k_AgentControlNames[control];
        EXPECT_EQ(static_cast<std::uint32_t>(*found), control);
    }

    EXPECT_FALSE(FindAgentControl("Jump").has_value());
    EXPECT_FALSE(FindAgentControl("").has_value());
    EXPECT_FALSE(FindAgentControl("sprint").has_value());
}

TEST(AgentTextTests, TheNamesAreTheLowercaseNamesOfTheControlsInOrder)
{
    const std::array<std::string_view, k_AgentControlCount> expected{ "jump", "crouch", "fire", "zoom", "zoom_in", "zoom_out", "melee", "ability",
        "throw_grenade", "use", "switch_weapon", "switch_grenade", "reload" };
    EXPECT_EQ(k_AgentControlNames, expected);
}

TEST(AgentTextTests, AFullLineSetsEveryField)
{
    AgentInput input{};
    float turn{ 0.0f };
    ASSERT_EQ(Parse({ "move=1,-0.5", "look=0.25,-0.125", "level=fire,crouch", "edge=jump,fire", "turn=0.1" }, input, turn), "");

    EXPECT_EQ(input.MoveForward, 1.0f);
    EXPECT_EQ(input.MoveLeft, -0.5f);
    EXPECT_EQ(input.YawDelta, 0.25f);
    EXPECT_EQ(input.PitchDelta, -0.125f);
    EXPECT_EQ(input.Level, Bit(static_cast<std::uint32_t>(AgentControl::Fire)) | Bit(static_cast<std::uint32_t>(AgentControl::Crouch)));
    EXPECT_EQ(input.Edge, Bit(static_cast<std::uint32_t>(AgentControl::Jump)) | Bit(static_cast<std::uint32_t>(AgentControl::Fire)));
    EXPECT_FLOAT_EQ(turn, 0.1f);
}

TEST(AgentTextTests, AKeyLeftOutKeepsWhatTheOutputHeldAndADashClearsTheControls)
{
    AgentInput input{ .MoveForward = 0.5f, .Level = 0x7u, .Edge = 0x3u };
    float turn{ 0.2f };
    ASSERT_EQ(Parse({ "look=1,2", "level=-" }, input, turn), "");

    EXPECT_EQ(input.MoveForward, 0.5f);
    EXPECT_EQ(input.YawDelta, 1.0f);
    EXPECT_EQ(input.Level, 0u);
    EXPECT_EQ(input.Edge, 0x3u);
    EXPECT_FLOAT_EQ(turn, 0.2f);
}

TEST(AgentTextTests, NoWordsChangesNothing)
{
    AgentInput input{ .MoveLeft = -1.0f, .Level = 0x10u };
    float turn{ 0.3f };
    EXPECT_EQ(Parse({}, input, turn), "");
    EXPECT_EQ(input.MoveLeft, -1.0f);
    EXPECT_EQ(input.Level, 0x10u);
    EXPECT_FLOAT_EQ(turn, 0.3f);
}

TEST(AgentTextTests, AnUnknownKeyOrControlOrAnUnshapedWordIsRefused)
{
    AgentInput input{};
    float turn{ 0.0f };
    EXPECT_NE(Parse({ "speed=1" }, input, turn), "");
    EXPECT_NE(Parse({ "level=jump,sprint" }, input, turn), "");
    EXPECT_NE(Parse({ "level=" }, input, turn), "");
    EXPECT_NE(Parse({ "level=jump," }, input, turn), "");
    EXPECT_NE(Parse({ "edge=Jump" }, input, turn), "");
    EXPECT_NE(Parse({ "jump" }, input, turn), "");
}

TEST(AgentTextTests, ARepeatedKeyIsRefused)
{
    AgentInput input{};
    float turn{ 0.0f };
    EXPECT_NE(Parse({ "move=1,0", "move=0,1" }, input, turn), "");
    EXPECT_NE(Parse({ "level=jump", "level=fire" }, input, turn), "");
}

TEST(AgentTextTests, NumbersMustBeFiniteAndInTheRightCount)
{
    AgentInput input{};
    float turn{ 0.0f };
    EXPECT_NE(Parse({ "move=1" }, input, turn), "");
    EXPECT_NE(Parse({ "move=1,0,0" }, input, turn), "");
    EXPECT_NE(Parse({ "move=a,b" }, input, turn), "");
    EXPECT_NE(Parse({ "move=nan,0" }, input, turn), "");
    EXPECT_NE(Parse({ "look=inf,0" }, input, turn), "");
    EXPECT_NE(Parse({ "look=1,2x" }, input, turn), "");
    EXPECT_NE(Parse({ "look=,2" }, input, turn), "");
    EXPECT_NE(Parse({ "turn=" }, input, turn), "");
    EXPECT_NE(Parse({ "turn=1,2" }, input, turn), "");
}

TEST(AgentTextTests, RangesAreChecked)
{
    AgentInput input{};
    float turn{ 0.0f };
    EXPECT_NE(Parse({ "move=1.5,0" }, input, turn), "");
    EXPECT_NE(Parse({ "move=0,-1.0001" }, input, turn), "");
    EXPECT_NE(Parse({ "turn=-0.1" }, input, turn), "");
    EXPECT_EQ(Parse({ "move=-1,1", "turn=0" }, input, turn), "");
    EXPECT_EQ(Parse({ "look=100,-100" }, input, turn), "");
    EXPECT_EQ(input.YawDelta, 100.0f);
}

TEST(AgentTextTests, AFailureLeavesTheInputAndTheTurnUntouched)
{
    AgentInput input{ .MoveForward = 0.25f, .YawDelta = 3.0f, .Level = 0x5u };
    float turn{ 0.5f };
    EXPECT_NE(Parse({ "move=1,1", "look=9,9", "level=jump", "edge=bogus" }, input, turn), "");

    EXPECT_EQ(input.MoveForward, 0.25f);
    EXPECT_EQ(input.YawDelta, 3.0f);
    EXPECT_EQ(input.Level, 0x5u);
    EXPECT_FLOAT_EQ(turn, 0.5f);
}

// The line 'input' of the bench without its players word (VW.9c.3 D): the first tick, the last tick, the window and the fields of the AgentInput.

namespace
{
    auto ParseLine(const std::vector<std::string>& words, AgentInputLine& line) -> std::string
    {
        return ParseAgentInputLine(std::span<const std::string>{ words }, line);
    }
}

TEST(AgentLineTests, ALineReadsTheRangeTheWindowAndTheFields)
{
    AgentInputLine line{};
    EXPECT_EQ(ParseLine({ "10", "0", "4", "move=1,-0.5", "look=0.4,0", "level=fire", "edge=fire,jump", "turn=0.02" }, line), "");
    EXPECT_EQ(line.FromTick, 10u);
    EXPECT_EQ(line.ToTick, 0u);
    EXPECT_EQ(line.WindowTicks, 4u);
    EXPECT_EQ(line.Input.MoveForward, 1.0f);
    EXPECT_EQ(line.Input.MoveLeft, -0.5f);
    EXPECT_EQ(line.Input.YawDelta, 0.4f);
    EXPECT_EQ(line.Input.Level, 1u << static_cast<std::uint32_t>(AgentControl::Fire));
    EXPECT_EQ(line.Input.Edge, (1u << static_cast<std::uint32_t>(AgentControl::Fire)) | (1u << static_cast<std::uint32_t>(AgentControl::Jump)));
    EXPECT_EQ(line.TurnLimit, 0.02f);

    EXPECT_EQ(ParseLine({ "5", "5", "1" }, line), "");
    EXPECT_EQ(line.Input.Level, 0u);
}

TEST(AgentLineTests, ALineWithoutTheRangeOrWithABadRangeOrWindowFailsAndKeepsTheOldLine)
{
    AgentInputLine line{ .FromTick = 3, .WindowTicks = 9 };

    EXPECT_NE(ParseLine({}, line), "");
    EXPECT_NE(ParseLine({ "10", "0" }, line), "");
    EXPECT_NE(ParseLine({ "ten", "0", "4" }, line), "");
    EXPECT_NE(ParseLine({ "10", "-1", "4" }, line), "");
    EXPECT_NE(ParseLine({ "10", "9", "4" }, line), "");
    EXPECT_NE(ParseLine({ "10", "20", "0" }, line), "");
    EXPECT_NE(ParseLine({ "10", "20", "4294967296" }, line), "");
    EXPECT_NE(ParseLine({ "10", "20", "4", "jump=1" }, line), "");
    EXPECT_NE(ParseLine({ "10", "20", "4", "move=2,0" }, line), "");

    EXPECT_EQ(line.FromTick, 3u);
    EXPECT_EQ(line.WindowTicks, 9u);
}

TEST(AgentLineTests, TwoRangesOverlapWhenTheyShareATickAndALastTickOfZeroIsTheEnd)
{
    EXPECT_TRUE(AreTickRangesOverlapping(10, 20, 20, 30));
    EXPECT_TRUE(AreTickRangesOverlapping(10, 20, 15, 16));
    EXPECT_FALSE(AreTickRangesOverlapping(10, 20, 21, 30));
    EXPECT_FALSE(AreTickRangesOverlapping(21, 30, 10, 20));
    EXPECT_TRUE(AreTickRangesOverlapping(10, 0, 500, 600));
    EXPECT_TRUE(AreTickRangesOverlapping(500, 600, 10, 0));
    EXPECT_TRUE(AreTickRangesOverlapping(10, 0, 20, 0));
    EXPECT_FALSE(AreTickRangesOverlapping(0, 9, 10, 0));
}
