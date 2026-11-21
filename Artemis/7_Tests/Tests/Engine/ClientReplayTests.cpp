#include <gtest/gtest.h>

import Control.Client.System;
import Control.Client.Type;
import Engine.Control.Input.System;
import Engine.Control.Input.Type;
import std;

// Calls the client oracle logged with a difference in the first in-game run (VW.9c.2, Ignore/Temporal/vw9c2A_oracle.txt), replayed through
// the pure client with the real bytes. The engine acts on the raw down byte only when it is 1 (the first frame of a press: 0x5d31c is
// compared with 1), the port acted on every frame the byte was not zero. No game needed.

namespace
{
    using Control::Client::System::StepClient;
    using Control::Client::Type::ClientSettings;
    using Control::Client::Type::ClientState;
    using Control::Client::Type::ClientStep;
    using Control::Client::Type::EventSample;
    using Control::Client::Type::InputEvent;
    using Control::Client::Type::InputFrame;
    using Control::Client::Type::UnitContext;
    using Control::Client::Type::k_EventZoomHeld;
    using Control::Client::Type::k_EventZoomPress;

    using Engine::Control::Input::System::ReadClientState;
    using Engine::Control::Input::System::ReadInputFrame;
    using Engine::Control::Input::Type::k_ClientBlockWindow;
    using Engine::Control::Input::Type::k_InputEventsOffset;
    using Engine::Control::Input::Type::k_InputEventSize;
    using Engine::Control::Input::Type::k_InputTableSize;

    // A patch of real bytes: the offset and the bytes in hex.
    struct Patch
    {
        std::size_t Offset{ 0 };
        std::string_view Hex{};
    };

    // A logged call: the dt and the rows of the table and of the client block that are not zero for the client. The rest reads as zero.
    struct ReplayLine
    {
        std::uint32_t Dt{ 0 };
        std::vector<Patch> Table{};
        std::vector<Patch> Block{};
    };

    auto Apply(std::span<std::uint8_t> bytes, const std::vector<Patch>& patches) -> void
    {
        for (const Patch& patch : patches)
        {
            for (std::size_t index{ 0 }; index + 1 < patch.Hex.size(); index += 2)
            {
                std::uint32_t value{ 0 };
                std::from_chars(patch.Hex.data() + index, patch.Hex.data() + index + 2, value, 16);
                bytes[patch.Offset + (index / 2)] = static_cast<std::uint8_t>(value);
            }
        }
    }

    // Replays the pure client over a logged call with the given unit context (every fact known).
    // param down: An event whose down byte is replaced (the logged calls have 2 or more), or nothing.
    auto ReplayStepClient(const ReplayLine& line, const UnitContext& unit,
        std::optional<std::pair<InputEvent, std::uint8_t>> down = std::nullopt) -> ClientStep
    {
        std::array<std::uint8_t, k_InputTableSize> table{};
        std::array<std::uint8_t, k_ClientBlockWindow> block{};
        Apply(table, line.Table);
        Apply(block, line.Block);
        if (down)
        {
            table[k_InputEventsOffset + (static_cast<std::size_t>(down->first) * k_InputEventSize) + 2] = down->second;
        }

        return StepClient(ReadInputFrame(table), ReadClientState(block), unit, ClientSettings{}, std::bit_cast<float>(line.Dt));
    }

    const ReplayLine k_LaneAbility{
        .Dt = 0x3C8B5E6D,
        .Table = {
            { 0x270, "00000000220002000000803F00000000" },
            { 0x480, "00000000220002000000803F00000000" },
            { 0x620, "00000000220002000000803F00000000" },
            { 0x6C0, "00000000000000800000008000000080" },
            { 0x6F0, "00000000000000000100000000000000" },
        },
        .Block = {
            { 0x10, "00100080C7544D40803BAA3F00000080" },
            { 0x40, "060001FFFFFFFFFFFFFFFFFF00000000" },
            { 0x150, "000000000000FFFF0000000000000000" },
        } };

    const ReplayLine k_ZoomPress{
        .Dt = 0x3C88889A,
        .Table = {
            { 0x2B0, "00000000220002000000803F00000000" },
            { 0x2C0, "00000000220002000000803F00000000" },
            { 0x350, "00000000220002000000803F00000000" },
            { 0x4A0, "00000000220002000000803F00000000" },
            { 0x530, "00000000220002000000803F00000000" },
            { 0x640, "00000000220002000000803F00000000" },
            { 0x6C0, "00000000000000800000008000000080" },
            { 0x6F0, "00000000000000000100000000000000" },
        },
        .Block = {
            { 0x10, "00000000C7544D40904EAA3F00000080" },
            { 0x30, "00000000000000000000803F00000000" },
            { 0x40, "060000FFFFFF0000FFFFFFFF00000000" },
            { 0x150, "000000000100FFFF0000000000000000" },
        } };

    const ReplayLine k_ZoomIn{
        .Dt = 0x3C88889A,
        .Table = {
            { 0x5E0, "00000000210002000000803F00000000" },
            { 0x610, "00000000210002000000803F00000000" },
            { 0x6C0, "00000000000000800000008000000080" },
            { 0x6F0, "00000000000000000100000000000000" },
        },
        .Block = {
            { 0x10, "00000000C7544D40904EAA3F00000080" },
            { 0x40, "060000FFFFFF0000FFFFFFFF00000000" },
            { 0x150, "000000000000FFFF7805CD3D00000000" },
        } };

    const ReplayLine k_ZoomOut{
        .Dt = 0x3C88889A,
        .Table = {
            { 0x460, "00000000210002000000803F00000000" },
            { 0x510, "00000000210002000000803F00000000" },
            { 0x5F0, "00000000210002000000803F00000000" },
            { 0x6C0, "00000000000000800000008000000080" },
            { 0x6F0, "00000000000000000100000000000000" },
        },
        .Block = {
            { 0x10, "00000000C7544D403548AA3F00000080" },
            { 0x40, "060000FFFFFFFFFFFFFFFFFF00000000" },
            { 0x150, "000000000000FFFF7805CD3D00000000" },
        } };
}

TEST(ClientReplayTests, TheZoomPressIsTheFirstFrameOfTheButtonNotEveryFrameItStaysDown)
{
    const UnitContext unit{ .HasUnit = true, .IsBiped = true };
    const ClientStep held{ ReplayStepClient(k_ZoomPress, unit) };
    EXPECT_EQ(held.Output.Events & k_EventZoomPress, 0u);
    EXPECT_NE(held.Output.Events & k_EventZoomHeld, 0u);

    const ClientStep first{ ReplayStepClient(k_ZoomPress, unit, std::pair{ InputEvent::Zoom, std::uint8_t{ 1 } }) };
    EXPECT_NE(first.Output.Events & k_EventZoomPress, 0u);
}

TEST(ClientReplayTests, TheZoomStepInIsTheFirstFrameOfTheButton)
{
    const UnitContext unit{ .HasUnit = true, .IsBiped = true };
    EXPECT_EQ(ReplayStepClient(k_ZoomIn, unit).Output.Events & (1u << 4), 0u);
    EXPECT_NE(ReplayStepClient(k_ZoomIn, unit, std::pair{ InputEvent::ZoomIn, std::uint8_t{ 1 } }).Output.Events & (1u << 4), 0u);
}

TEST(ClientReplayTests, TheZoomStepOutIsTheFirstFrameOfTheButton)
{
    const UnitContext unit{ .HasUnit = true, .IsBiped = true };
    EXPECT_EQ(ReplayStepClient(k_ZoomOut, unit).Output.Events & (1u << 5), 0u);
    EXPECT_NE(ReplayStepClient(k_ZoomOut, unit, std::pair{ InputEvent::ZoomOut, std::uint8_t{ 1 } }).Output.Events & (1u << 5), 0u);
}

TEST(ClientReplayTests, TheAbilityLaneNextToTheSprintIsTheFirstFrameOfTheButton)
{
    UnitContext unit{ .HasUnit = true, .IsBiped = true };
    unit.Ability.IsPresent = true;
    unit.Ability.Kind = 0;

    const ClientStep held{ ReplayStepClient(k_LaneAbility, unit) };
    EXPECT_EQ(held.Output.Lanes & (1u << 31), 0u);
    EXPECT_NE(held.Output.Lanes & (1u << 12), 0u);

    const ClientStep first{ ReplayStepClient(k_LaneAbility, unit, std::pair{ InputEvent::Ability, std::uint8_t{ 1 } }) };
    EXPECT_NE(first.Output.Lanes & (1u << 31), 0u);
}

TEST(ClientReplayTests, TheCrouchToggleFlipsOnTheFirstFrameOnlyAndAHoldKeepsTheState)
{
    const UnitContext unit{ .HasUnit = true, .IsBiped = true };
    ClientSettings settings{};
    settings.IsCrouchToggle = true;

    ClientState state{};
    state.IsCrouchToggled = true;
    InputFrame frame{};
    frame.Events[static_cast<std::size_t>(InputEvent::Crouch)] = EventSample{ .IsDown = true, .DownCount = 3, .HeldMs = 100, .Value = 1.0f };
    EXPECT_TRUE(StepClient(frame, state, unit, settings, 1.0f / 60.0f).State.IsCrouchToggled);

    frame.Events[static_cast<std::size_t>(InputEvent::Crouch)].DownCount = 1;
    EXPECT_FALSE(StepClient(frame, state, unit, settings, 1.0f / 60.0f).State.IsCrouchToggled);

    settings.IsCrouchToggle = false;
    state.IsCrouchToggled = false;
    frame.Events[static_cast<std::size_t>(InputEvent::Crouch)].DownCount = 3;
    EXPECT_FALSE(StepClient(frame, state, unit, settings, 1.0f / 60.0f).State.IsCrouchToggled);
    frame.Events[static_cast<std::size_t>(InputEvent::Crouch)].DownCount = 1;
    EXPECT_TRUE(StepClient(frame, state, unit, settings, 1.0f / 60.0f).State.IsCrouchToggled);
}