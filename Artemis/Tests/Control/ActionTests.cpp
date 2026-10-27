#include <gtest/gtest.h>

import Common.Math.System;
import Control.Action.Type;
import Control.Action.System;
import Layout.Action.Type;
import std;

// Control.Action (M6a, D210-D213): the action an agent gets each tick, written over the empty action of the engine. The tests fix what
// the code does today: the facing, the buttons, the hold lanes and the weapon, grenade and zoom requests.

namespace
{
    using Common::Math::System::TurnToward;
    using Common::Math::System::MoveToward;

    using Control::Action::Type::ActionOverride;
    using Control::Action::Type::ClientRequests;
    using Control::Action::Type::PlayerActionHistory;
    using Control::Action::Type::UnitClientState;
    using Control::Action::System::UnitRules;
    using Control::Action::System::BuildAction;
    using Control::Action::System::GetHoldThresholdTicks;

    using Layout::Action::Type::k_ActionKeepZoomButton;
    using Layout::Action::Type::k_HoldLanes;
    using Layout::Action::Type::k_UseLane;
    using Layout::Action::Type::Size::k_PlayerActionSize;
    using Layout::Action::Type::Offset::k_ActionButtonsOffset;
    using Layout::Action::Type::Offset::k_ActionFacingOffset;
    using Layout::Action::Type::Offset::k_ActionThrottleOffset;
    using Layout::Action::Type::Offset::k_ActionLanesOffset;
    using Layout::Action::Type::Offset::k_ActionInventoryVersionOffset;
    using Layout::Action::Type::Offset::k_ActionWeaponSlotOffset;
    using Layout::Action::Type::Offset::k_ActionGrenadeTypeOffset;
    using Layout::Action::Type::Offset::k_ActionZoomLevelOffset;

    using Action = std::array<std::uint8_t, k_PlayerActionSize>;

    // Bytes a test checks were not written.
    constexpr std::uint8_t k_Untouched{ 0xCD };

    // Value of the requests in the empty action of the engine: they ask for nothing.
    constexpr std::uint16_t k_NoRequest16{ 0xFFFF };
    constexpr std::uint8_t k_NoWeaponRequest{ 0xFF };

    // return: An action of 0xCD bytes, except the requests as the engine leaves them when it clears the action.
    auto MakeAction() -> Action
    {
        Action action{};
        action.fill(k_Untouched);

        const std::uint16_t none{ k_NoRequest16 };
        std::memcpy(action.data() + k_ActionInventoryVersionOffset, &none, sizeof(none));
        action[k_ActionWeaponSlotOffset] = k_NoWeaponRequest;
        std::memcpy(action.data() + k_ActionGrenadeTypeOffset, &none, sizeof(none));
        std::memcpy(action.data() + k_ActionZoomLevelOffset, &none, sizeof(none));
        return action;
    }

    template<typename T>
    auto ReadAt(const Action& action, std::uintptr_t offset) -> T
    {
        T value{};
        std::memcpy(&value, action.data() + offset, sizeof(value));
        return value;
    }

    auto ReadButtons(const Action& action) -> std::uint64_t { return ReadAt<std::uint64_t>(action, k_ActionButtonsOffset); }
    auto ReadYaw(const Action& action) -> float { return ReadAt<float>(action, k_ActionFacingOffset); }
    auto ReadPitch(const Action& action) -> float { return ReadAt<float>(action, k_ActionFacingOffset + sizeof(float)); }
    auto ReadThrottleForward(const Action& action) -> float { return ReadAt<float>(action, k_ActionThrottleOffset); }
    auto ReadThrottleLeft(const Action& action) -> float { return ReadAt<float>(action, k_ActionThrottleOffset + sizeof(float)); }
    auto ReadLanes(const Action& action) -> std::uint32_t { return ReadAt<std::uint32_t>(action, k_ActionLanesOffset); }
    auto ReadInventoryVersion(const Action& action) -> std::uint16_t { return ReadAt<std::uint16_t>(action, k_ActionInventoryVersionOffset); }
    auto ReadWeaponSlot(const Action& action) -> std::uint8_t { return action[k_ActionWeaponSlotOffset]; }
    auto ReadGrenadeType(const Action& action) -> std::uint16_t { return ReadAt<std::uint16_t>(action, k_ActionGrenadeTypeOffset); }
    auto ReadZoomLevel(const Action& action) -> std::uint16_t { return ReadAt<std::uint16_t>(action, k_ActionZoomLevelOffset); }

    // The engine questions, answered by a rule the test can change. Counts the calls and keeps the last state it was asked about.
    // By default: the next weapon or grenade type up to 3, the next zoom level up to 2, then nothing.
    class FakeUnitRules : public UnitRules
    {
    public:
        mutable int WeaponCalls{ 0 };
        mutable int GrenadeCalls{ 0 };
        mutable int ZoomCalls{ 0 };
        mutable UnitClientState LastWeaponState{};

        // When set, the answer of that question (nullopt included) instead of the default rule.
        std::optional<std::optional<std::int8_t>> FixedWeapon{};
        std::optional<std::optional<std::int8_t>> FixedGrenade{};
        std::optional<std::optional<std::int8_t>> FixedZoom{};

        auto GetNextWeaponSlot(const UnitClientState& state) const -> std::optional<std::int8_t> override
        {
            ++WeaponCalls;
            LastWeaponState = state;
            if (FixedWeapon) return *FixedWeapon;
            if (state.WeaponSlot < 3) return static_cast<std::int8_t>(state.WeaponSlot + 1);
            return std::nullopt;
        }

        auto GetNextGrenadeType(const UnitClientState& state) const -> std::optional<std::int8_t> override
        {
            ++GrenadeCalls;
            if (FixedGrenade) return *FixedGrenade;
            if (state.GrenadeType < 3) return static_cast<std::int8_t>(state.GrenadeType + 1);
            return std::nullopt;
        }

        auto GetNextZoomLevel(const UnitClientState& state) const -> std::optional<std::int8_t> override
        {
            ++ZoomCalls;
            if (FixedZoom) return *FixedZoom;
            if (state.ZoomLevel < 2) return static_cast<std::int8_t>(state.ZoomLevel + 1);
            return std::nullopt;
        }
    };

    // return: The unit the tests start from: slot 0, inventory version 7, no zoom, facing 0.
    auto MakeUnit() -> UnitClientState
    {
        return UnitClientState{ .Unit = 0x0001'0002, .WeaponSlot = 0, .InventoryVersion = 7, .GrenadeType = 0, .ZoomLevel = -1,
            .IsSeated = false, .Facing = { 0.0f, 0.0f } };
    }

    // One agent over several ticks: the history lives between them, and every tick starts from a new empty action as the engine does.
    struct Agent
    {
        ActionOverride Override{};
        std::optional<UnitClientState> Unit{ MakeUnit() };
        std::optional<std::uint32_t> HoldTicks{};
        ClientRequests Requests{};
        std::uint64_t Sequence{ 0 };
        FakeUnitRules Rules{};
        PlayerActionHistory History{};

        Action Last{};
        std::array<float, 2> LastFacing{};

        // Runs one tick. The result is in Last and LastFacing.
        auto Tick() -> void
        {
            Last = MakeAction();
            LastFacing = BuildAction(std::span<std::uint8_t, k_PlayerActionSize>{ Last.data(), k_PlayerActionSize }, Override, Unit,
                HoldTicks, Requests, Sequence, Rules, History);
        }
    };

    // Whether bits of `value` are all set.
    auto HasBits(std::uint64_t value, std::uint64_t bits) -> bool
    {
        return (value & bits) == bits;
    }
}

TEST(ActionTests, HoldThresholdTicksRoundsUp)
{
    EXPECT_EQ(GetHoldThresholdTicks(0.25f), 15u);
    EXPECT_EQ(GetHoldThresholdTicks(0.017f), 2u);
    EXPECT_EQ(GetHoldThresholdTicks(0.016f), 1u);
    EXPECT_EQ(GetHoldThresholdTicks(4.999f), 300u);

    EXPECT_FALSE(GetHoldThresholdTicks(0.0f).has_value());
    EXPECT_FALSE(GetHoldThresholdTicks(-1.0f).has_value());
    EXPECT_FALSE(GetHoldThresholdTicks(5.0f).has_value());
    EXPECT_FALSE(GetHoldThresholdTicks(std::numeric_limits<float>::quiet_NaN()).has_value());
    EXPECT_FALSE(GetHoldThresholdTicks(std::numeric_limits<float>::infinity()).has_value());
}

TEST(ActionTests, ButtonsKeepZoomAndThrottle)
{
    Agent agent{};
    agent.Override.Buttons = 1ull | (1ull << 30);
    agent.Override.ThrottleForward = 0.5f;
    agent.Override.ThrottleLeft = -0.25f;
    agent.Tick();

    EXPECT_EQ(ReadButtons(agent.Last), (1ull | (1ull << 30)) | (1ull << 25));
    EXPECT_EQ(k_ActionKeepZoomButton, 1ull << 25);
    EXPECT_FLOAT_EQ(ReadThrottleForward(agent.Last), 0.5f);
    EXPECT_FLOAT_EQ(ReadThrottleLeft(agent.Last), -0.25f);

    EXPECT_EQ(ReadInventoryVersion(agent.Last), 0xFFFF);
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 0xFF);
    EXPECT_EQ(ReadGrenadeType(agent.Last), 0xFFFF);
    EXPECT_EQ(ReadZoomLevel(agent.Last), 0xFFFF);
}

TEST(ActionTests, OnlyItsFieldsAreWritten)
{
    Agent agent{};
    agent.Override.Buttons = 0xFFu;
    agent.Override.Yaw = 1.0f;
    agent.Override.Pitch = 0.5f;
    agent.Override.ThrottleForward = 1.0f;
    agent.Override.ThrottleLeft = 1.0f;
    agent.Override.Lanes = k_UseLane;
    agent.HoldTicks = 1;
    agent.Requests = ClientRequests{ .IsWeaponSwitch = true, .IsGrenadeSwitch = true, .IsZoom = true };
    agent.Tick();

    const auto isUntouched = [&agent](std::size_t first, std::size_t last)
    {
        for (std::size_t offset{ first }; offset < last; ++offset)
        {
            if (agent.Last[offset] != k_Untouched) return false;
        }
        return true;
    };

    EXPECT_TRUE(isUntouched(0x00, 0x08));
    EXPECT_TRUE(isUntouched(0x18, 0x1C));
    EXPECT_TRUE(isUntouched(0x24, 0x40));
    EXPECT_TRUE(isUntouched(0x4C, 0xA8));
}

TEST(ActionTests, NoUnitWritesTheTarget)
{
    Agent agent{};
    agent.Unit = std::nullopt;
    agent.Override.TurnLimit = 0.1f;
    agent.Override.Yaw = 1.0f;
    agent.Override.Pitch = 0.2f;
    agent.Tick();

    EXPECT_FLOAT_EQ(ReadYaw(agent.Last), 1.0f);
    EXPECT_FLOAT_EQ(ReadPitch(agent.Last), 0.2f);
    EXPECT_FLOAT_EQ(agent.LastFacing[0], 1.0f);
    EXPECT_FLOAT_EQ(agent.LastFacing[1], 0.2f);
    EXPECT_FALSE(agent.History.Facing.has_value());

    agent.Requests.IsWeaponSwitch = true;
    agent.Tick();

    EXPECT_EQ(ReadInventoryVersion(agent.Last), 0xFFFF);
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 0xFF);
    EXPECT_FALSE(agent.History.PendingWeapon.has_value());
    EXPECT_EQ(agent.Rules.WeaponCalls, 0);
    EXPECT_EQ(agent.Rules.GrenadeCalls, 0);
    EXPECT_EQ(agent.Rules.ZoomCalls, 0);
}

TEST(ActionTests, TurnLimitZeroGoesStraight)
{
    Agent agent{};
    agent.Unit = MakeUnit();
    agent.Unit->Facing = { 3.0f, 0.0f };
    agent.Override.Yaw = 1.0f;
    agent.Override.Pitch = 0.5f;
    agent.Override.TurnLimit = 0.0f;
    agent.Tick();

    EXPECT_FLOAT_EQ(ReadYaw(agent.Last), 1.0f);
    EXPECT_FLOAT_EQ(ReadPitch(agent.Last), 0.5f);
}

TEST(ActionTests, TurnStepsFromTheUnitThenFromTheLastFacing)
{
    Agent agent{};
    agent.Override.Yaw = 1.0f;
    agent.Override.Pitch = 0.5f;
    agent.Override.TurnLimit = 0.1f;

    agent.Tick();
    EXPECT_FLOAT_EQ(ReadYaw(agent.Last), 0.1f);
    EXPECT_FLOAT_EQ(ReadPitch(agent.Last), 0.1f);

    // The same unit goes on from the facing written, although its own facing has not moved.
    agent.Tick();
    EXPECT_FLOAT_EQ(ReadYaw(agent.Last), 0.2f);
    EXPECT_FLOAT_EQ(ReadPitch(agent.Last), 0.2f);

    // Another unit starts from its own facing.
    agent.Unit->Unit = 0x0001'0003;
    agent.Tick();
    EXPECT_FLOAT_EQ(ReadYaw(agent.Last), 0.1f);
    EXPECT_FLOAT_EQ(ReadPitch(agent.Last), 0.1f);
}

TEST(ActionTests, YawTurnsTheShortWay)
{
    Agent agent{};
    agent.Unit->Facing = { 6.2f, 0.0f };
    agent.Override.Yaw = 0.1f;
    agent.Override.Pitch = 0.5f;
    agent.Override.TurnLimit = 0.1f;
    agent.Tick();

    EXPECT_FLOAT_EQ(ReadYaw(agent.Last), TurnToward(6.2f, 0.1f, 0.1f));
    EXPECT_LT(ReadYaw(agent.Last), 0.1f);
    EXPECT_FLOAT_EQ(ReadPitch(agent.Last), MoveToward(0.0f, 0.5f, 0.1f));
}

TEST(ActionTests, LaneHeldAfterTheThreshold)
{
    constexpr std::uint32_t use{ 1u << 20 };
    constexpr std::uint32_t useHeld{ 1u << 21 };
    constexpr std::uint32_t lane5{ 1u << 27 };
    constexpr std::uint32_t lane5Held{ 1u << 29 };

    Agent agent{};
    agent.HoldTicks = 3;
    agent.Override.Lanes = use;

    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use);
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use);
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use | useHeld);
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use | useHeld);

    // One tick up resets the count.
    agent.Override.Lanes = 0;
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), 0u);

    agent.Override.Lanes = use;
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use);

    // Every lane counts by itself: lane 5 joins while use has been down for two ticks.
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use);
    agent.Override.Lanes = use | lane5;
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use | useHeld | lane5);
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use | useHeld | lane5);
    agent.Tick();
    EXPECT_EQ(ReadLanes(agent.Last), use | useHeld | lane5 | lane5Held);
}

TEST(ActionTests, LaneNeverHeldWithoutThreshold)
{
    Agent agent{};
    agent.HoldTicks = std::nullopt;
    agent.Override.Lanes = k_UseLane;

    for (int tick{ 0 }; tick < 10; ++tick)
    {
        agent.Tick();
        EXPECT_EQ(ReadLanes(agent.Last), k_UseLane);
        EXPECT_FALSE(HasBits(ReadLanes(agent.Last), std::uint64_t{ 1 } << k_HoldLanes[0].HeldBit));
    }
}

TEST(ActionTests, UseTestCycles)
{
    Agent agent{};
    agent.Override.IsUseTest = true;
    agent.Override.Lanes = 0;

    agent.Sequence = 59;
    agent.Tick();
    EXPECT_TRUE(HasBits(ReadLanes(agent.Last), k_UseLane));

    agent.Sequence = 60;
    agent.Tick();
    EXPECT_FALSE(HasBits(ReadLanes(agent.Last), k_UseLane));

    agent.Sequence = 63;
    agent.Tick();
    EXPECT_FALSE(HasBits(ReadLanes(agent.Last), k_UseLane));

    agent.Sequence = 64;
    agent.Tick();
    EXPECT_TRUE(HasBits(ReadLanes(agent.Last), k_UseLane));
}

TEST(ActionTests, WeaponRequestIsSentUntilHeld)
{
    Agent agent{};
    agent.Requests.IsWeaponSwitch = true;

    for (int tick{ 1 }; tick <= 30; ++tick)
    {
        agent.Tick();
        agent.Requests.IsWeaponSwitch = false;

        EXPECT_EQ(ReadInventoryVersion(agent.Last), 7) << "tick " << tick;
        EXPECT_EQ(ReadWeaponSlot(agent.Last), 1) << "tick " << tick;
        EXPECT_EQ(ReadZoomLevel(agent.Last), 0xFFFF) << "tick " << tick;
        EXPECT_FALSE(HasBits(ReadButtons(agent.Last), k_ActionKeepZoomButton)) << "tick " << tick;
    }

    agent.Tick();
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 0xFF);
    EXPECT_TRUE(HasBits(ReadButtons(agent.Last), k_ActionKeepZoomButton));
}

TEST(ActionTests, WeaponRequestDropsWhenHeldOrInventoryChanges)
{
    Agent agent{};
    agent.Requests.IsWeaponSwitch = true;
    agent.Tick();
    agent.Requests.IsWeaponSwitch = false;
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 1);

    // The unit holds the slot now.
    agent.Unit->WeaponSlot = 1;
    agent.Tick();
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 0xFF);
    EXPECT_FALSE(agent.History.PendingWeapon.has_value());

    // A new request goes on from the slot in the hands, then the inventory changes under it.
    agent.Requests.IsWeaponSwitch = true;
    agent.Tick();
    agent.Requests.IsWeaponSwitch = false;
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 2);

    agent.Unit->InventoryVersion = 8;
    agent.Tick();
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 0xFF);
    EXPECT_FALSE(agent.History.PendingWeapon.has_value());
}

TEST(ActionTests, SecondPressGoesOnFromThePending)
{
    Agent agent{};
    agent.Requests.IsWeaponSwitch = true;
    agent.Tick();
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 1);

    // The unit is still in slot 0.
    agent.Tick();
    EXPECT_EQ(agent.Rules.LastWeaponState.WeaponSlot, 1);
    EXPECT_EQ(ReadWeaponSlot(agent.Last), 2);
}

TEST(ActionTests, ZoomRequest)
{
    Agent agent{};
    agent.Requests.IsZoom = true;
    agent.Tick();

    EXPECT_EQ(ReadZoomLevel(agent.Last), 0x0000);
    EXPECT_FALSE(HasBits(ReadButtons(agent.Last), k_ActionKeepZoomButton));
    EXPECT_EQ(agent.Rules.ZoomCalls, 1);

    // The unit cannot zoom now: the action keeps the zoom of the unit.
    agent.Rules.FixedZoom = std::optional<std::int8_t>{};
    agent.Tick();

    EXPECT_EQ(ReadZoomLevel(agent.Last), 0xFFFF);
    EXPECT_TRUE(HasBits(ReadButtons(agent.Last), k_ActionKeepZoomButton));
}

TEST(ActionTests, ZoomDroppedDuringWeaponSwitch)
{
    Agent agent{};
    agent.Requests = ClientRequests{ .IsWeaponSwitch = true, .IsZoom = true };
    agent.Tick();

    EXPECT_EQ(ReadWeaponSlot(agent.Last), 1);
    EXPECT_EQ(ReadZoomLevel(agent.Last), 0xFFFF);
    EXPECT_EQ(agent.Rules.ZoomCalls, 0);
}

TEST(ActionTests, GrenadeRequest)
{
    Agent agent{};
    agent.Requests.IsGrenadeSwitch = true;
    agent.Tick();

    EXPECT_EQ(ReadGrenadeType(agent.Last), 0x0001);
    EXPECT_EQ(agent.Rules.GrenadeCalls, 1);

    // There is no other grenade type: the request stays empty.
    agent.Rules.FixedGrenade = std::optional<std::int8_t>{};
    agent.Tick();

    EXPECT_EQ(ReadGrenadeType(agent.Last), 0xFFFF);
}