#include <gtest/gtest.h>

import Control.Action.Type;
import Control.Client.Type;
import Engine.Control.Simulation.System;
import Engine.Control.Simulation.Type;
import std;

// The unit facts of the client resolved from bytes (VW.9c.2, D356): a fake port gives objects, tags and engine calls. No game needed.

namespace
{
    using Control::Action::Type::UnitClientState;
    using Control::Client::Type::ClientContextSource;
    using Control::Client::Type::UnitContext;
    using Control::Client::Type::UnitFact;
    using Control::Client::Type::k_AllUnitFacts;

    using Engine::Control::Simulation::System::BuildUnitContext;
    using Control::Client::Type::ClientSettings;
    using Engine::Control::Simulation::System::UnitFactPort;
    using Engine::Control::Simulation::Type::DecodeTagGlobals;
    using Engine::Control::Simulation::Type::TagGlobals;

    constexpr std::uint32_t k_Unit{ 0x0001'0001 };
    constexpr std::uint32_t k_Weapon{ 0x0002'0002 };
    constexpr std::uint32_t k_Held{ 0x0003'0003 };
    constexpr std::uint32_t k_Parent{ 0x0004'0004 };
    constexpr std::uint32_t k_Player{ 0x0005'0005 };
    constexpr std::uint32_t k_Block{ 0x0000'0100 };
    constexpr std::size_t k_ObjectSize{ 0xB00 };
    constexpr std::size_t k_TagSize{ 0x800 };

    // Bytes of objects, tags, blocks and players, and the answers of the engine functions. A function with no answer is a function the
    // port does not have.
    class FakePort final : public UnitFactPort
    {
    public:
        std::map<std::uint32_t, std::vector<std::uint8_t>> Objects{};
        std::map<std::uint32_t, std::uint8_t> Types{};
        std::map<std::uint32_t, std::vector<std::uint8_t>> Players{};
        std::map<std::uint32_t, std::vector<std::uint8_t>> Tags{};
        std::map<std::uint32_t, std::vector<std::uint8_t>> Blocks{};
        std::optional<bool> Feature{ false };

        std::optional<float> Magnification{};
        std::optional<float> Penalty{};
        std::optional<bool> CrouchGate{};
        std::optional<bool> ForcedCrouch{};
        std::optional<bool> CrouchToggleBlocked{};
        std::optional<std::int16_t> NextZoom{};
        std::optional<std::uint32_t> Frame{};
        std::optional<bool> ControlsLocked{};
        std::optional<bool> LookOffset{};
        std::optional<bool> AltControl{};

        mutable std::int16_t LastMagnificationLevel{ 0 };
        mutable std::uint32_t LastZoomWeapon{ 0 };
        mutable std::vector<std::array<bool, 2>> PenaltyCalls{};

        auto ReadObject(std::uint32_t handle, std::size_t offset, std::span<std::uint8_t> bytes) const -> bool override
        {
            return Copy(Objects, handle, offset, bytes);
        }

        auto GetObjectType(std::uint32_t handle) const -> std::optional<std::uint8_t> override
        {
            const auto found{ Types.find(handle) };
            if (found == Types.end()) return std::nullopt;
            return found->second;
        }

        auto ReadPlayer(std::uint32_t handle, std::size_t offset, std::span<std::uint8_t> bytes) const -> bool override
        {
            return Copy(Players, handle, offset, bytes);
        }

        auto ReadTag(std::uint32_t tag, std::size_t offset, std::span<std::uint8_t> bytes) const -> bool override
        {
            return Copy(Tags, tag & 0xFFFF, offset, bytes);
        }

        auto ReadBlock(std::uint32_t index, std::size_t offset, std::span<std::uint8_t> bytes) const -> bool override
        {
            return Copy(Blocks, index, offset, bytes);
        }

        auto IsFeatureOn(std::uint32_t) const -> std::optional<bool> override { return Feature; }

        auto GetZoomMagnification(std::uint32_t, std::int16_t level) const -> std::optional<float> override
        {
            LastMagnificationLevel = level;
            return Magnification;
        }

        auto GetMovePenalty(std::uint32_t, bool isForward, bool isZoomed) const -> std::optional<float> override
        {
            PenaltyCalls.push_back({ isForward, isZoomed });
            if (!Penalty) return std::nullopt;
            return isForward ? *Penalty : *Penalty / 2.0f;
        }

        auto IsCrouchGate(std::uint32_t) const -> std::optional<bool> override { return CrouchGate; }
        auto IsForcedCrouch(std::uint32_t) const -> std::optional<bool> override { return ForcedCrouch; }
        auto IsCrouchToggleBlocked(std::uint32_t) const -> std::optional<bool> override { return CrouchToggleBlocked; }
        auto GetNextZoomLevel(std::uint32_t weapon) const -> std::optional<std::int16_t> override
        {
            LastZoomWeapon = weapon;
            return NextZoom;
        }
        auto GetFrameOfReference(std::uint32_t) const -> std::optional<std::uint32_t> override { return Frame; }
        auto IsControlsLocked(std::int32_t, std::uint32_t) const -> std::optional<bool> override { return ControlsLocked; }
        auto IsLookOffset(std::uint32_t, float) const -> std::optional<bool> override { return LookOffset; }
        auto IsAltControl(std::uint32_t) const -> std::optional<bool> override { return AltControl; }

    private:
        static auto Copy(const std::map<std::uint32_t, std::vector<std::uint8_t>>& memory, std::uint32_t key, std::size_t offset,
            std::span<std::uint8_t> bytes) -> bool
        {
            const auto found{ memory.find(key) };
            if (found == memory.end() || offset + bytes.size() > found->second.size()) return false;

            std::memcpy(bytes.data(), found->second.data() + offset, bytes.size());
            return true;
        }
    };

    template<typename T>
    auto Put(std::vector<std::uint8_t>& bytes, std::size_t offset, T value) -> void
    {
        std::memcpy(bytes.data() + offset, &value, sizeof(T));
    }

    // A unit on foot with a weapon in slot 0 and nothing in its hands. Every engine function answers "no".
    auto MakePort() -> FakePort
    {
        FakePort port{};
        port.Feature = false;
        port.Magnification = 3.0f;
        port.Penalty = 0.0f;
        port.CrouchGate = false;
        port.ForcedCrouch = false;
        port.CrouchToggleBlocked = false;
        port.NextZoom = 0;
        port.Frame = 0xFFFFFFFF;
        port.ControlsLocked = false;
        port.LookOffset = false;
        port.AltControl = false;

        std::vector<std::uint8_t> unit(k_ObjectSize, 0);
        Put<std::uint16_t>(unit, 0x00, 0x0010);
        Put<std::uint32_t>(unit, 0x14, 0xFFFFFFFF);
        Put<std::int16_t>(unit, 0x336, -1);
        Put<std::int8_t>(unit, 0x34A, 0);
        Put<std::int8_t>(unit, 0x34B, -1);
        Put<std::uint32_t>(unit, 0x350, k_Weapon);
        Put<std::uint32_t>(unit, 0x374, 0xFFFFFFFF);
        Put<std::int16_t>(unit, 0x1B2, 0x400);
        Put<std::int16_t>(unit, 0x18E, 0x500);
        port.Objects[k_Unit] = unit;
        port.Types[k_Unit] = 0;

        std::vector<std::uint8_t> weapon(k_ObjectSize, 0);
        Put<std::uint16_t>(weapon, 0x00, 0x0020);
        port.Objects[k_Weapon] = weapon;
        port.Types[k_Weapon] = 2;

        port.Tags[0x0010] = std::vector<std::uint8_t>(k_TagSize, 0);
        port.Tags[0x0020] = std::vector<std::uint8_t>(k_TagSize, 0);
        port.Players[k_Player] = std::vector<std::uint8_t>(0x400, 0);
        return port;
    }

    auto MakeState() -> UnitClientState
    {
        return UnitClientState{ .Unit = k_Unit, .WeaponSlot = 0, .InventoryVersion = 1, .GrenadeType = 0, .ZoomLevel = -1, .IsSeated = false };
    }

    auto MakeSource() -> ClientContextSource
    {
        return ClientContextSource{ .ThreadContext = 1, .InputUser = 0, .LocalIndex = 0, .PlayerHandle = k_Player };
    }

    // Builds with the default settings (60 ticks per second) and a dt of 1/60.
    auto Build(const UnitFactPort& port, const UnitClientState& state, const ClientContextSource& source) -> UnitContext
    {
        return BuildUnitContext(port, state, source, ClientSettings{}, 1.0f / 60.0f);
    }

    auto IsKnown(const UnitContext& context, UnitFact fact) -> bool
    {
        return (context.Known & static_cast<std::uint32_t>(fact)) != 0;
    }

    // Puts a seat parent in the world: the unit sits in seat 1 of a vehicle whose tag has the given kind and seat flags.
    auto SeatUnit(FakePort& port, std::uint32_t kindIndex, std::uint32_t seatFlags, std::uint8_t altByte) -> void
    {
        Put<std::uint32_t>(port.Objects[k_Unit], 0x14, k_Parent);
        Put<std::int16_t>(port.Objects[k_Unit], 0x336, 1);

        std::vector<std::uint8_t> parent(k_ObjectSize, 0);
        Put<std::uint16_t>(parent, 0x00, 0x0040);
        Put<std::uint32_t>(parent, 0x14, 0xFFFFFFFF);
        Put<std::uint8_t>(parent, 0x8E, altByte);
        port.Objects[k_Parent] = parent;
        port.Types[k_Parent] = 1;

        std::vector<std::uint8_t> tag(k_TagSize, 0);
        Put<std::uint32_t>(tag, 0x4D0 + (kindIndex * 12), 1);
        Put<std::int32_t>(tag, 0x444, 3);
        Put<std::uint32_t>(tag, 0x448, k_Block);
        port.Tags[0x0040] = tag;

        std::vector<std::uint8_t> block(3 * 0x12C, 0);
        Put<std::uint32_t>(block, 1 * 0x12C, seatFlags);
        port.Blocks[k_Block] = block;
    }
}

TEST(UnitContextTests, AnUnitOnFootResolvesEveryFact)
{
    const FakePort port{ MakePort() };
    const UnitContext context{ Build(port, MakeState(), MakeSource()) };

    EXPECT_EQ(context.Known, k_AllUnitFacts);
    EXPECT_TRUE(context.HasUnit);
    EXPECT_TRUE(context.IsBiped);
    EXPECT_FALSE(context.IsSeated);
}

TEST(UnitContextTests, WithoutAUnitTheUnitFactsAreKnownAsTheirDefault)
{
    const FakePort port{ MakePort() };
    UnitClientState state{ MakeState() };
    state.Unit = 0xFFFFFFFF;

    const UnitContext context{ Build(port, state, MakeSource()) };
    EXPECT_FALSE(context.HasUnit);
    EXPECT_EQ(context.Known, k_AllUnitFacts);
}

TEST(UnitContextTests, TheObjectTypeDecidesBiped)
{
    FakePort port{ MakePort() };
    port.Types[k_Unit] = 1;

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_FALSE(context.IsBiped);
    EXPECT_TRUE(IsKnown(context, UnitFact::Biped));
    EXPECT_TRUE(IsKnown(context, UnitFact::CrouchGate));
    EXPECT_TRUE(IsKnown(context, UnitFact::FrameOfReference));
}

TEST(UnitContextTests, TheSeatFlagsAndTheParentKindComeFromTheParentTag)
{
    FakePort port{ MakePort() };
    SeatUnit(port, 2, 0x4, 1);
    UnitClientState state{ MakeState() };
    state.IsSeated = true;

    const UnitContext context{ Build(port, state, MakeSource()) };
    EXPECT_TRUE(context.IsSeatFlagged);
    EXPECT_TRUE(context.IsParentKind2);
    EXPECT_TRUE(IsKnown(context, UnitFact::SeatFlags));
    EXPECT_EQ(context.Known, k_AllUnitFacts);
}

TEST(UnitContextTests, ASeatWithoutTheFlagIsNotFlaggedAndAKindOtherThanTwoIsNot)
{
    FakePort port{ MakePort() };
    SeatUnit(port, 5, 0x0, 1);
    UnitClientState state{ MakeState() };
    state.IsSeated = true;

    const UnitContext context{ Build(port, state, MakeSource()) };
    EXPECT_FALSE(context.IsSeatFlagged);
    EXPECT_FALSE(context.IsParentKind2);
    EXPECT_FALSE(context.IsAltPitch);
}

TEST(UnitContextTests, TheAlternatePitchNeedsTheByteTheKindAndTheFlag)
{
    UnitClientState state{ MakeState() };
    state.IsSeated = true;

    FakePort allowed{ MakePort() };
    SeatUnit(allowed, 5, 0x4, 1);
    EXPECT_TRUE(Build(allowed, state, MakeSource()).IsAltPitch);

    FakePort wrongByte{ MakePort() };
    SeatUnit(wrongByte, 5, 0x4, 2);
    EXPECT_FALSE(Build(wrongByte, state, MakeSource()).IsAltPitch);

    FakePort wrongKind{ MakePort() };
    SeatUnit(wrongKind, 3, 0x4, 1);
    EXPECT_FALSE(Build(wrongKind, state, MakeSource()).IsAltPitch);

    FakePort noFlag{ MakePort() };
    SeatUnit(noFlag, 5, 0x0, 1);
    EXPECT_FALSE(Build(noFlag, state, MakeSource()).IsAltPitch);
}

TEST(UnitContextTests, ASeatOutsideTheTableIsNotFlagged)
{
    FakePort port{ MakePort() };
    SeatUnit(port, 2, 0x4, 1);
    Put<std::int16_t>(port.Objects[k_Unit], 0x336, 7);
    UnitClientState state{ MakeState() };
    state.IsSeated = true;

    const UnitContext context{ Build(port, state, MakeSource()) };
    EXPECT_FALSE(context.IsSeatFlagged);
    EXPECT_TRUE(IsKnown(context, UnitFact::SeatFlags));
}

TEST(UnitContextTests, TheMagnificationIsAskedOnlyWithAZoomLevelAndWithThatLevel)
{
    FakePort port{ MakePort() };
    ClientContextSource source{ MakeSource() };

    const UnitContext notZoomed{ Build(port, MakeState(), source) };
    EXPECT_EQ(notZoomed.ZoomMagnification, 0.0f);
    EXPECT_TRUE(IsKnown(notZoomed, UnitFact::ZoomMagnification));

    source.ClientZoomLevel = 2;
    const UnitContext zoomed{ Build(port, MakeState(), source) };
    EXPECT_EQ(zoomed.ZoomMagnification, 3.0f);
    EXPECT_EQ(port.LastMagnificationLevel, 2);
}

TEST(UnitContextTests, ThePenaltyOfTheWeaponGoesToTheTwoAxesWithTheZoomState)
{
    FakePort port{ MakePort() };
    port.Penalty = 0.4f;
    ClientContextSource source{ MakeSource() };
    source.ClientZoomLevel = 0;

    const UnitContext context{ Build(port, MakeState(), source) };
    EXPECT_EQ(context.MovePenaltyForward, 0.4f);
    EXPECT_EQ(context.MovePenaltySide, 0.2f);
    ASSERT_EQ(port.PenaltyCalls.size(), 2u);
    EXPECT_TRUE(port.PenaltyCalls[0][0]);
    EXPECT_TRUE(port.PenaltyCalls[0][1]);
    EXPECT_FALSE(port.PenaltyCalls[1][0]);
}

TEST(UnitContextTests, WithoutAWeaponThereIsNoPenaltyAndNoZoomItem)
{
    FakePort port{ MakePort() };
    port.Penalty = 0.4f;
    Put<std::int8_t>(port.Objects[k_Unit], 0x34A, -1);

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_EQ(context.MovePenaltyForward, 0.0f);
    EXPECT_FALSE(context.HasZoomItem);
    EXPECT_TRUE(IsKnown(context, UnitFact::MovePenalty));
    EXPECT_TRUE(IsKnown(context, UnitFact::ZoomItem));
    EXPECT_TRUE(port.PenaltyCalls.empty());
}

TEST(UnitContextTests, AWeaponWithoutZoomLevelsIsAZoomItemWithoutLevels)
{
    FakePort port{ MakePort() };
    port.NextZoom = -1;

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(context.HasZoomItem);
    EXPECT_TRUE(context.IsZoomItemWithoutLevels);

    port.NextZoom = 0;
    EXPECT_FALSE(Build(port, MakeState(), MakeSource()).IsZoomItemWithoutLevels);
}

TEST(UnitContextTests, TheWeaponTagGivesTheCrouchFlagAndTheTriggerFlag)
{
    FakePort port{ MakePort() };
    Put<std::uint32_t>(port.Tags[0x0020], 0x204, (1u << 12));

    const UnitContext crouch{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(crouch.IsWeaponCrouchFlag);
    EXPECT_FALSE(crouch.IsWeaponTriggerFlag);

    Put<std::uint32_t>(port.Tags[0x0020], 0x204, (1u << 18));
    const UnitContext trigger{ Build(port, MakeState(), MakeSource()) };
    EXPECT_FALSE(trigger.IsWeaponCrouchFlag);
    EXPECT_TRUE(trigger.IsWeaponTriggerFlag);
}

TEST(UnitContextTests, DualWieldNeedsBothWeaponSlots)
{
    FakePort port{ MakePort() };
    EXPECT_FALSE(Build(port, MakeState(), MakeSource()).IsDualWield);

    Put<std::int8_t>(port.Objects[k_Unit], 0x34B, 1);
    ClientContextSource source{ MakeSource() };
    source.DualWieldFlags = 6;

    const UnitContext context{ Build(port, MakeState(), source) };
    EXPECT_TRUE(context.IsDualWield);
    EXPECT_EQ(context.DualWieldFlags, 6);
}

TEST(UnitContextTests, TheCarriedObjectDropsOnlyWhileItsStateIsSet)
{
    FakePort port{ MakePort() };
    Put<std::uint32_t>(port.Objects[k_Unit], 0x374, k_Held);

    std::vector<std::uint8_t> held(k_ObjectSize, 0);
    Put<std::uint16_t>(held, 0x00, 0x0050);
    Put<std::uint32_t>(held, 0x1D8, 0x1234);
    port.Objects[k_Held] = held;

    std::vector<std::uint8_t> tag(k_TagSize, 0);
    Put<std::uint32_t>(tag, 0x28C + (0x16 * 12), 1);
    port.Tags[0x0050] = tag;

    const UnitContext carried{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(carried.IsCarriedDrop);
    EXPECT_FALSE(carried.IsCarriedZoomReset);
    EXPECT_TRUE(carried.Ability.IsPresent);

    Put<std::uint32_t>(port.Objects[k_Held], 0x1D8, 0xFFFFFFFF);
    EXPECT_FALSE(Build(port, MakeState(), MakeSource()).IsCarriedDrop);
}

TEST(UnitContextTests, TheZoomResetFlagIsSeparateFromTheDropFlag)
{
    FakePort port{ MakePort() };
    Put<std::uint32_t>(port.Objects[k_Unit], 0x374, k_Held);

    std::vector<std::uint8_t> held(k_ObjectSize, 0);
    Put<std::uint16_t>(held, 0x00, 0x0050);
    Put<std::uint32_t>(held, 0x1D8, 7);
    port.Objects[k_Held] = held;

    std::vector<std::uint8_t> tag(k_TagSize, 0);
    Put<std::uint32_t>(tag, 0x28C + (0x14 * 12), 1);
    port.Tags[0x0050] = tag;

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_FALSE(context.IsCarriedDrop);
    EXPECT_TRUE(context.IsCarriedZoomReset);
}

TEST(UnitContextTests, TheAbilityKindAndTheEngineFlagComeFromTheHeldObjectTag)
{
    FakePort port{ MakePort() };
    Put<std::uint32_t>(port.Objects[k_Unit], 0x374, k_Held);

    std::vector<std::uint8_t> held(k_ObjectSize, 0);
    Put<std::uint16_t>(held, 0x00, 0x0050);
    Put<std::uint32_t>(held, 0x1D8, 0xFFFFFFFF);
    port.Objects[k_Held] = held;

    std::vector<std::uint8_t> tag(k_TagSize, 0);
    Put<std::uint8_t>(tag, 0x24A, 1);
    Put<std::uint32_t>(tag, 0x28C + (0x11 * 12), 5);
    port.Tags[0x0050] = tag;

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(context.Ability.IsPresent);
    EXPECT_EQ(context.Ability.Kind, 1);
    EXPECT_TRUE(context.Ability.NeedsEngine);
    EXPECT_TRUE(IsKnown(context, UnitFact::Ability));
}

TEST(UnitContextTests, TheUnitTagGivesTheTriggerCurveAndTheAimDamping)
{
    FakePort port{ MakePort() };
    Put<std::uint32_t>(port.Tags[0x0010], 0x168, (1u << 27));
    Put<float>(port.Tags[0x0010], 0x304, 0.5f);
    Put<float>(port.Objects[k_Unit], 0x4D4, 0.25f);

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(context.IsTriggerCurve);
    EXPECT_EQ(context.AimDampTag, 0.5f);
    EXPECT_EQ(context.AimDampUnit, 0.25f);
}

TEST(UnitContextTests, TheCrouchFactsCombineTheCallsAndTheKindWord)
{
    FakePort port{ MakePort() };
    port.CrouchGate = true;
    port.ForcedCrouch = true;

    const UnitContext calls{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(calls.IsCrouchGate);
    EXPECT_TRUE(calls.IsForcedCrouch);
    EXPECT_FALSE(calls.IsCrouchToggleBlocked);

    Put<std::uint32_t>(port.Objects[k_Unit], 0x500 + 0x160, 4);
    Put<std::uint32_t>(port.Objects[k_Unit], 0x500 + 0x204, 0xB9);
    EXPECT_TRUE(Build(port, MakeState(), MakeSource()).IsCrouchToggleBlocked);

    Put<std::uint32_t>(port.Objects[k_Unit], 0x500 + 0x160, 0);
    EXPECT_FALSE(Build(port, MakeState(), MakeSource()).IsCrouchToggleBlocked);

    port.CrouchToggleBlocked = true;
    EXPECT_TRUE(Build(port, MakeState(), MakeSource()).IsCrouchToggleBlocked);
}

TEST(UnitContextTests, TheFrameOfReferenceIsAHandleThatIsNotNone)
{
    FakePort port{ MakePort() };
    port.Frame = 0x00070007;

    EXPECT_TRUE(Build(port, MakeState(), MakeSource()).IsFrameOfReference);
}

TEST(UnitContextTests, TheSelectionListIsTheOneOfTheRootAndItsType)
{
    FakePort port{ MakePort() };
    Put<std::int32_t>(port.Tags[0x0010], 0x4F4, 3);

    const UnitContext biped{ Build(port, MakeState(), MakeSource()) };
    EXPECT_EQ(biped.SelectionEntries, 3u);

    Put<std::int32_t>(port.Tags[0x0010], 0x4F4, 0);
    EXPECT_EQ(Build(port, MakeState(), MakeSource()).SelectionEntries, 0u);

    SeatUnit(port, 2, 0x4, 1);
    Put<std::int32_t>(port.Tags[0x0040], 0x5D8, 2);
    UnitClientState state{ MakeState() };
    state.IsSeated = true;

    const UnitContext vehicle{ Build(port, state, MakeSource()) };
    EXPECT_EQ(vehicle.SelectionEntries, 2u);
    EXPECT_TRUE(IsKnown(vehicle, UnitFact::SelectionEntries));
}

TEST(UnitContextTests, TheJumpAssistNeedsTheFeatureAndReadsTheFlagsTheCounterAndTheState)
{
    FakePort port{ MakePort() };
    ClientContextSource source{ MakeSource() };

    const UnitContext off{ Build(port, MakeState(), source) };
    EXPECT_FALSE(off.JumpAssist.Applies);
    EXPECT_TRUE(IsKnown(off, UnitFact::JumpAssist));

    port.Feature = true;
    Put<std::uint32_t>(port.Objects[k_Unit], 0x400, 1);
    Put<std::uint64_t>(port.Objects[k_Unit], 0x1D8, (1ull << 1) | (1ull << 0xE));
    Put<std::int8_t>(port.Objects[k_Unit], 0xA29, 3);

    const UnitContext on{ Build(port, MakeState(), source) };
    EXPECT_TRUE(on.JumpAssist.Applies);
    EXPECT_TRUE(on.JumpAssist.HasObject);
    EXPECT_TRUE(on.JumpAssist.IsStateOne);
    EXPECT_TRUE(on.JumpAssist.IsFlagA);
    EXPECT_TRUE(on.JumpAssist.IsFlagB);
    EXPECT_EQ(on.JumpAssist.Counter, 3);
    EXPECT_EQ(on.JumpAssist.LimitHalf, 2);
}

TEST(UnitContextTests, ASeatWithTheFlagBit2TakesTheTriggerCurveFromTheParentTag)
{
    UnitClientState state{ MakeState() };
    state.IsSeated = true;

    FakePort flagged{ MakePort() };
    SeatUnit(flagged, 5, 0x4, 1);
    Put<std::uint32_t>(flagged.Tags[0x0040], 0x168, (1u << 27));
    const UnitContext fromParent{ Build(flagged, state, MakeSource()) };
    EXPECT_TRUE(fromParent.IsTriggerCurve);
    EXPECT_TRUE(IsKnown(fromParent, UnitFact::TriggerCurve));

    FakePort plain{ MakePort() };
    SeatUnit(plain, 5, 0x0, 1);
    Put<std::uint32_t>(plain.Tags[0x0040], 0x168, (1u << 27));
    EXPECT_FALSE(Build(plain, state, MakeSource()).IsTriggerCurve);

    Put<std::uint32_t>(plain.Tags[0x0010], 0x168, (1u << 27));
    EXPECT_TRUE(Build(plain, state, MakeSource()).IsTriggerCurve);

    Put<std::uint32_t>(flagged.Tags[0x0040], 0x168, 0);
    Put<std::uint32_t>(flagged.Tags[0x0010], 0x168, (1u << 27));
    EXPECT_FALSE(Build(flagged, state, MakeSource()).IsTriggerCurve);
}

TEST(UnitContextTests, ASeatWithTheFlagBit3AsksTheZoomItemOfTheWeaponOfTheParent)
{
    constexpr std::uint32_t k_ParentWeapon{ 0x0006'0006 };
    UnitClientState state{ MakeState() };
    state.IsSeated = true;

    FakePort port{ MakePort() };
    SeatUnit(port, 5, 0x8, 1);
    Put<std::int8_t>(port.Objects[k_Parent], 0x34A, 0);
    Put<std::uint32_t>(port.Objects[k_Parent], 0x350, k_ParentWeapon);
    port.NextZoom = -1;

    const UnitContext parent{ Build(port, state, MakeSource()) };
    EXPECT_EQ(port.LastZoomWeapon, k_ParentWeapon);
    EXPECT_TRUE(parent.HasZoomItem);
    EXPECT_TRUE(parent.IsZoomItemWithoutLevels);

    Put<std::int8_t>(port.Objects[k_Parent], 0x34A, -1);
    const UnitContext none{ Build(port, state, MakeSource()) };
    EXPECT_FALSE(none.HasZoomItem);
    EXPECT_TRUE(IsKnown(none, UnitFact::ZoomItem));

    FakePort own{ MakePort() };
    SeatUnit(own, 5, 0x4, 1);
    Put<std::uint32_t>(own.Objects[k_Parent], 0x350, k_ParentWeapon);
    Put<std::int8_t>(own.Objects[k_Parent], 0x34A, 0);
    const UnitContext ownWeapon{ Build(own, state, MakeSource()) };
    EXPECT_EQ(own.LastZoomWeapon, k_Weapon);
    EXPECT_TRUE(ownWeapon.HasZoomItem);
}

TEST(UnitContextTests, TheZoomItemStaysUnknownWhenTheSeatFlagCannotBeRead)
{
    UnitClientState state{ MakeState() };
    state.IsSeated = true;

    FakePort port{ MakePort() };
    SeatUnit(port, 5, 0x8, 1);
    port.Tags.erase(0x0040);
    const UnitContext context{ Build(port, state, MakeSource()) };
    EXPECT_FALSE(IsKnown(context, UnitFact::ZoomItem));
    EXPECT_FALSE(IsKnown(context, UnitFact::TriggerCurve));
    EXPECT_TRUE(IsKnown(context, UnitFact::MovePenalty));
}

TEST(UnitContextTests, TheJumpAssistHasNoObjectForAUnitThatIsNotABiped)
{
    FakePort port{ MakePort() };
    port.Feature = true;
    port.Types[k_Unit] = 1;
    Put<std::uint32_t>(port.Objects[k_Unit], 0x400, 1);

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(context.JumpAssist.Applies);
    EXPECT_FALSE(context.JumpAssist.HasObject);
    EXPECT_TRUE(IsKnown(context, UnitFact::JumpAssist));

    port.Types.erase(k_Unit);
    EXPECT_FALSE(IsKnown(Build(port, MakeState(), MakeSource()), UnitFact::JumpAssist));
}

TEST(UnitContextTests, WithoutTheRestrictionsByteDualWieldStaysUnknown)
{
    FakePort port{ MakePort() };
    ClientContextSource source{ MakeSource() };
    source.DualWieldFlags = std::nullopt;
    EXPECT_FALSE(IsKnown(Build(port, MakeState(), source), UnitFact::DualWield));

    UnitClientState noUnit{ MakeState() };
    noUnit.Unit = 0xFFFFFFFF;
    EXPECT_FALSE(IsKnown(Build(port, noUnit, source), UnitFact::DualWield));

    source.DualWieldFlags = 2;
    const UnitContext known{ Build(port, MakeState(), source) };
    EXPECT_TRUE(IsKnown(known, UnitFact::DualWield));
    EXPECT_EQ(known.DualWieldFlags, 2);
    EXPECT_TRUE(IsKnown(Build(port, noUnit, source), UnitFact::DualWield));
}

TEST(UnitContextTests, TheTicksPerSecondOfTheSettingsGiveTheJumpAssistLimit)
{
    FakePort port{ MakePort() };
    port.Feature = true;
    ClientSettings settings{};
    settings.TicksPerSecond = 30;
    const UnitContext context{ BuildUnitContext(port, MakeState(), MakeSource(), settings, 0.0f) };
    EXPECT_EQ(context.JumpAssist.LimitHalf, 1);
}

TEST(UnitContextTests, TheUserFactsComeFromThePlayerAndTheCalls)
{
    FakePort port{ MakePort() };
    std::vector<std::uint8_t> player(0x400, 0);
    Put<std::uint8_t>(player, 0x310, 1);
    port.Players[k_Player] = player;
    port.ControlsLocked = true;
    port.LookOffset = true;
    port.AltControl = true;

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(context.IsLookRated);
    EXPECT_TRUE(context.IsControlLocked);
    EXPECT_TRUE(context.IsLookOffset);
    EXPECT_TRUE(context.IsAltControl);
    EXPECT_EQ(context.Known, k_AllUnitFacts);
}

TEST(UnitContextTests, APlayerWithoutHandleHasNoLookRatesNorAlternateControl)
{
    FakePort port{ MakePort() };
    ClientContextSource source{ MakeSource() };
    source.PlayerHandle = 0xFFFFFFFF;

    const UnitContext context{ Build(port, MakeState(), source) };
    EXPECT_FALSE(context.IsLookRated);
    EXPECT_FALSE(context.IsAltControl);
    EXPECT_TRUE(IsKnown(context, UnitFact::LookRated));
}

TEST(UnitContextTests, AFactWithoutItsFunctionOrMemoryStaysUnknown)
{
    FakePort port{ MakePort() };
    port.Magnification.reset();
    port.Penalty.reset();
    port.CrouchGate.reset();
    port.ControlsLocked.reset();
    port.Tags.erase(0x0010);
    ClientContextSource source{ MakeSource() };
    source.ClientZoomLevel = 1;

    const UnitContext context{ Build(port, MakeState(), source) };
    EXPECT_FALSE(IsKnown(context, UnitFact::ZoomMagnification));
    EXPECT_FALSE(IsKnown(context, UnitFact::MovePenalty));
    EXPECT_FALSE(IsKnown(context, UnitFact::CrouchGate));
    EXPECT_FALSE(IsKnown(context, UnitFact::ControlLocked));
    EXPECT_FALSE(IsKnown(context, UnitFact::TriggerCurve));
    EXPECT_FALSE(IsKnown(context, UnitFact::AimDamp));
    EXPECT_FALSE(IsKnown(context, UnitFact::SelectionEntries));
    EXPECT_TRUE(IsKnown(context, UnitFact::HasUnit));
    EXPECT_TRUE(IsKnown(context, UnitFact::Carried));
}

TEST(UnitContextTests, ADeadUnitObjectLeavesTheUnitFactsUnknown)
{
    FakePort port{ MakePort() };
    port.Feature = true;
    port.Objects.erase(k_Unit);
    port.Types.erase(k_Unit);

    const UnitContext context{ Build(port, MakeState(), MakeSource()) };
    EXPECT_TRUE(IsKnown(context, UnitFact::HasUnit));
    EXPECT_FALSE(IsKnown(context, UnitFact::Biped));
    EXPECT_FALSE(IsKnown(context, UnitFact::DualWield));
    EXPECT_FALSE(IsKnown(context, UnitFact::Carried));
    EXPECT_FALSE(IsKnown(context, UnitFact::JumpAssist));
}

TEST(UnitContextTests, TheTagGlobalsAreDecodedFromTheTwoInstructions)
{
    std::array<std::uint8_t, 36> bytes{};
    bytes[0] = 0x48;
    bytes[1] = 0x8B;
    bytes[2] = 0x05;
    const std::int32_t mapDisplacement{ 0x100 };
    std::memcpy(bytes.data() + 3, &mapDisplacement, sizeof(mapDisplacement));
    bytes[29] = 0x48;
    bytes[30] = 0x8D;
    bytes[31] = 0x05;
    const std::int32_t segmentDisplacement{ -0x20 };
    std::memcpy(bytes.data() + 32, &segmentDisplacement, sizeof(segmentDisplacement));

    const std::optional<TagGlobals> globals{ DecodeTagGlobals(0x1000, bytes) };
    ASSERT_TRUE(globals.has_value());
    EXPECT_EQ(globals->Map, 0x1000u + 7 + 0x100);
    EXPECT_EQ(globals->Segments, 0x1000u + 29 + 7 - 0x20);

    bytes[1] = 0x00;
    EXPECT_FALSE(DecodeTagGlobals(0x1000, bytes).has_value());
    EXPECT_FALSE(DecodeTagGlobals(0x1000, std::span<const std::uint8_t>{ bytes.data(), 10 }).has_value());
}