#include <gtest/gtest.h>

import Control.Client.Type;
import Engine.Control.Input.System;
import Engine.Control.Input.Type;
import std;

// The bytes of the event table and of the client block as the client oracle reads them (VW.9c.1, D356). No game needed.

namespace
{
    using Control::Client::Type::ClientState;
    using Control::Client::Type::ClientStep;
    using Control::Client::Type::InputEvent;
    using Control::Client::Type::InputFrame;
    using Control::Client::Type::UnitContext;

    using Engine::Control::Input::System::CompareClient;
    using Engine::Control::Input::System::CountClient;
    using Engine::Control::Input::System::FormatClientLine;
    using Engine::Control::Input::System::FormatClientStateFields;
    using Engine::Control::Input::System::ReadClientState;
    using Engine::Control::Input::System::ReadInputFrame;
    using Engine::Control::Input::Type::ClientCounts;
    using Engine::Control::Input::Type::ClientDifferences;
    using Engine::Control::Input::Type::ClientStateField;
    using Engine::Control::Input::Type::ShadowField;
    using Engine::Control::Input::Type::k_ClientBlockWindow;
    using Engine::Control::Input::Type::k_ClientGrenadeOffset;
    using Engine::Control::Input::Type::k_ClientCrouchOffset;
    using Engine::Control::Input::Type::k_ClientOutputSize;
    using Engine::Control::Input::Type::k_EventLinkAny;
    using Engine::Control::Input::Type::k_InputEventsOffset;
    using Engine::Control::Input::Type::k_InputEventSize;
    using Engine::Control::Input::Type::k_InputMouseFlagOffset;
    using Engine::Control::Input::Type::k_InputMoveForwardOffset;
    using Engine::Control::Input::Type::k_InputTableSize;

    // Writes an event entry: held ms, down, flags, value and the link that makes it valid.
    auto SetEvent(std::array<std::uint8_t, k_InputTableSize>& table, InputEvent event, std::uint16_t heldMs, bool isDown, float value,
        std::uint8_t flags = 0) -> void
    {
        const std::size_t offset{ k_InputEventsOffset + (static_cast<std::size_t>(event) * k_InputEventSize) };
        std::memcpy(table.data() + offset, &heldMs, sizeof(heldMs));
        table[offset + 2] = isDown ? 1 : 0;
        table[offset + 3] = flags;
        std::memcpy(table.data() + offset + 4, &value, sizeof(value));
        const std::uint32_t link{ k_EventLinkAny };
        std::memcpy(table.data() + offset + 0xC, &link, sizeof(link));
    }

    auto MakeTable() -> std::array<std::uint8_t, k_InputTableSize>
    {
        std::array<std::uint8_t, k_InputTableSize> table{};
        table[k_InputMouseFlagOffset] = 1;
        return table;
    }
}

TEST(ClientFrameTests, AnEventEntryIsReadAsDownHeldAndValue)
{
    auto table{ MakeTable() };
    SetEvent(table, InputEvent::Fire, 217, true, 0.5f);
    const float forward{ 1.0f };
    std::memcpy(table.data() + k_InputMoveForwardOffset, &forward, sizeof(forward));

    const InputFrame frame{ ReadInputFrame(table) };
    const auto& fire{ frame.Events[static_cast<std::size_t>(InputEvent::Fire)] };
    EXPECT_TRUE(frame.IsMouse);
    EXPECT_EQ(frame.Mode, 1);
    EXPECT_TRUE(fire.IsDown);
    EXPECT_EQ(fire.HeldMs, 217);
    EXPECT_EQ(fire.Value, 0.5f);
    EXPECT_EQ(frame.MoveForward, 1.0f);
}

TEST(ClientFrameTests, ABlockedEntryReadsAsZero)
{
    auto table{ MakeTable() };
    SetEvent(table, InputEvent::Use, 500, true, 1.0f, 1);

    const InputFrame frame{ ReadInputFrame(table) };
    const auto& use{ frame.Events[static_cast<std::size_t>(InputEvent::Use)] };
    EXPECT_FALSE(use.IsDown);
    EXPECT_EQ(use.HeldMs, 0);
    EXPECT_EQ(use.Value, 0.0f);
}

TEST(ClientFrameTests, AnEntryWithAnotherLinkReadsAsZero)
{
    auto table{ MakeTable() };
    SetEvent(table, InputEvent::Jump, 10, true, 1.0f);
    const std::size_t offset{ k_InputEventsOffset };
    const std::uint32_t linkAny{ 5 };
    const std::uint32_t link{ 6 };
    std::memcpy(table.data() + offset + 0xC, &linkAny, sizeof(linkAny));
    std::memcpy(table.data() + offset + 0x8, &link, sizeof(link));

    EXPECT_FALSE(ReadInputFrame(table).Events[0].IsDown);
}

TEST(ClientFrameTests, WithoutTheMouseFlagTheModeIsNotKeyboardAndMouse)
{
    std::array<std::uint8_t, k_InputTableSize> table{};
    const InputFrame frame{ ReadInputFrame(table) };
    EXPECT_FALSE(frame.IsMouse);
    EXPECT_EQ(frame.Mode, 0);
}

TEST(ClientFrameTests, TheClientBlockStateIsReadByItsOffsets)
{
    std::array<std::uint8_t, k_ClientBlockWindow> block{};
    block[k_ClientCrouchOffset] = 1;
    const std::int16_t zoom{ 2 };
    std::memcpy(block.data() + 0x46, &zoom, sizeof(zoom));

    const ClientState state{ ReadClientState(block) };
    EXPECT_TRUE(state.IsCrouchToggled);
    EXPECT_EQ(state.ZoomLevel, 2);
}

TEST(ClientFrameTests, TheComparisonNamesTheBitsThatDiffer)
{
    ClientStep step{};
    step.Output.Buttons = (1ull << 30) | (1ull << 1);
    step.Output.Lanes = 1u << 20;
    step.State.IsCrouchToggled = true;

    std::array<std::uint8_t, k_ClientOutputSize> output{};
    const std::uint64_t buttons{ 1ull << 30 };
    std::memcpy(output.data() + 0x20, &buttons, sizeof(buttons));
    const std::uint32_t lanes{ 1u << 20 };
    std::memcpy(output.data() + 0x2C, &lanes, sizeof(lanes));

    std::array<std::uint8_t, k_ClientBlockWindow> block{};
    const auto differences{ CompareClient(step, output, block) };

    EXPECT_EQ(differences.Buttons, 1ull << 1);
    EXPECT_EQ(differences.Lanes, 0u);
    EXPECT_EQ(differences.Events, 0u);
    EXPECT_FALSE(differences.IsMove);
    EXPECT_NE(differences.StateFields, 0u);
    EXPECT_FALSE(differences.IsEmpty());
}

TEST(ClientFrameTests, EqualOutputAndStateGiveNoDifference)
{
    const ClientStep step{};
    std::array<std::uint8_t, k_ClientOutputSize> output{};
    std::array<std::uint8_t, k_ClientBlockWindow> block{};
    const std::int16_t noZoom{ -1 };
    std::memcpy(block.data() + 0x46, &noZoom, sizeof(noZoom));
    const std::uint32_t none{ 0xFFFFFFFF };
    std::memcpy(block.data() + 0x48, &none, sizeof(none));
    std::memcpy(block.data() + 0x114, &none, sizeof(none));

    EXPECT_TRUE(CompareClient(step, output, block).IsEmpty());
}

TEST(ClientFrameTests, ACallThatReadUnknownFactsOnlyCountsAsPartial)
{
    ClientStep step{};
    step.Output.UnknownRead = 0x80;
    ClientDifferences differences{};
    differences.IsMove = true;

    const ClientCounts counts{ CountClient(step, differences, UnitContext{ .HasUnit = true }, false) };

    ASSERT_EQ(counts.Size, 1u);
    EXPECT_EQ(counts.Items[0].Field, ShadowField::ClientPartial);
    EXPECT_FALSE(counts.Items[0].IsMatch);
}

TEST(ClientFrameTests, APartialCallThatMatchesCountsAsAPartialMatch)
{
    ClientStep step{};
    step.Output.UnknownRead = 0x4;

    const ClientCounts counts{ CountClient(step, ClientDifferences{}, UnitContext{}, true) };

    ASSERT_EQ(counts.Size, 1u);
    EXPECT_EQ(counts.Items[0].Field, ShadowField::ClientPartial);
    EXPECT_TRUE(counts.Items[0].IsMatch);
}

TEST(ClientFrameTests, AFullyKnownCallFillsTheFieldsTheContextAndTheZoom)
{
    ClientDifferences differences{};
    differences.Lanes = 0x10;

    const ClientCounts counts{ CountClient(ClientStep{}, differences, UnitContext{ .HasUnit = true, .IsSeated = true }, true) };

    ASSERT_EQ(counts.Size, 9u);
    EXPECT_EQ(counts.Items[0].Field, ShadowField::ClientMove);
    EXPECT_TRUE(counts.Items[0].IsMatch);
    EXPECT_EQ(counts.Items[5].Field, ShadowField::ClientLanes);
    EXPECT_FALSE(counts.Items[5].IsMatch);
    EXPECT_EQ(counts.Items[7].Field, ShadowField::ClientSeated);
    EXPECT_FALSE(counts.Items[7].IsMatch);
    EXPECT_EQ(counts.Items[8].Field, ShadowField::ClientZoomed);
}

TEST(ClientFrameTests, TheContextFieldIsNoUnitOnFootOrSeated)
{
    EXPECT_EQ(CountClient(ClientStep{}, ClientDifferences{}, UnitContext{}, false).Items[7].Field, ShadowField::ClientNoUnit);
    EXPECT_EQ(CountClient(ClientStep{}, ClientDifferences{}, UnitContext{ .HasUnit = true }, false).Items[7].Field, ShadowField::ClientOnFoot);
}

TEST(ClientFrameTests, TheFileLineCarriesTheMaskTheDtAndTheBytes)
{
    ClientDifferences differences{};
    differences.Events = 0x100;
    differences.StateFields = 0x2;
    const std::array<std::uint8_t, 3> table{ 0x01, 0xAB, 0xFF };
    const std::array<std::uint8_t, 2> block{ 0x00, 0x7E };

    const std::string line{ FormatClientLine(1, 1.0f, differences, 0x84, table, block) };

    EXPECT_EQ(line, "diff user=1 dt=3F800000 unknown=84 move=false look=false trig=false tail=false buttons=0 events=100 lanes=0 state=2"
        " table=01ABFF block=007E");
}

TEST(ClientFrameTests, TheStateFieldsThatDifferAreNamed)
{
    ClientDifferences differences{};
    EXPECT_EQ(FormatClientStateFields(differences), " none");

    differences.StateFields = (1u << static_cast<std::uint32_t>(ClientStateField::Zoom)) | (1u << static_cast<std::uint32_t>(ClientStateField::Crouch));
    EXPECT_EQ(FormatClientStateFields(differences), " zoom level crouch");
}

TEST(ClientFrameTests, TheBlockWindowCoversTheHighestFieldThePortReads)
{
    EXPECT_GE(k_ClientBlockWindow, k_ClientGrenadeOffset + 4);
}
