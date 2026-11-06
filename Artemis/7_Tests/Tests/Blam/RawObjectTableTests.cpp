#include <gtest/gtest.h>

import Blam.Layout.Snapshot.Type;
import std;

// RawObjectTable of Blam.Layout (S2.C, D276): the objects of a Snapshot in one arena with an index by slot.

namespace
{
    using Blam::Layout::Snapshot::Type::RawObject;
    using Blam::Layout::Snapshot::Type::RawObjectTable;

    constexpr std::uint32_t k_HandleA{ 0x0001'0005 };
    constexpr std::uint32_t k_HandleB{ 0x0002'0007 };

    auto Pattern(std::size_t size, std::uint8_t seed) -> std::vector<std::uint8_t>
    {
        std::vector<std::uint8_t> bytes(size);
        for (std::size_t i{ 0 }; i < size; ++i) bytes[i] = static_cast<std::uint8_t>(seed + (i * 3));
        return bytes;
    }

    auto Has(const RawObjectTable& table, std::uint32_t handle, const std::vector<std::uint8_t>& bytes) -> bool
    {
        const RawObject* found{ table.Find(handle) };
        return found != nullptr && found->Handle == handle && std::ranges::equal(found->Bytes, bytes);
    }
}

TEST(RawObjectTableTests, FindChecksTheWholeHandleSaltIncluded)
{
    RawObjectTable table{};
    ASSERT_TRUE(table.Add(k_HandleA, 0x1000, 3, Pattern(64, 1)));

    EXPECT_NE(table.Find(k_HandleA), nullptr);
    EXPECT_TRUE(table.Contains(k_HandleA));
    EXPECT_EQ(table.Find(0x0002'0005), nullptr);
    EXPECT_EQ(table.Find(k_HandleB), nullptr);
    EXPECT_EQ(table.Find(0x0001'FFFF), nullptr);
    EXPECT_EQ(table.Find(0xFFFF'FFFF), nullptr);
    EXPECT_EQ(table.Find(k_HandleA)->Address, 0x1000u);
    EXPECT_EQ(table.Find(k_HandleA)->KindId, 3u);
}

TEST(RawObjectTableTests, SlotOutsideTheEngineTableOrTakenIsRefused)
{
    RawObjectTable table{};
    ASSERT_TRUE(table.Add(k_HandleA, 0, 0, Pattern(16, 1)));

    EXPECT_FALSE(table.Add(0x0002'0005, 0, 0, Pattern(16, 2)));
    EXPECT_FALSE(table.Add(0x0001'0800, 0, 0, Pattern(16, 3)));
    EXPECT_FALSE(table.Emplace(0x0001'FFFF, 0, 0, 16).has_value());
    EXPECT_EQ(table.Size(), 1u);
    EXPECT_TRUE(Has(table, k_HandleA, Pattern(16, 1)));
}

TEST(RawObjectTableTests, IterationFollowsTheOrderOfAdding)
{
    RawObjectTable table{};
    table.Add(0x0001'0002, 0, 0, Pattern(8, 1));
    table.Add(0x0001'0009, 0, 0, Pattern(8, 2));
    table.Add(0x0003'0004, 0, 0, Pattern(8, 3));

    std::vector<std::uint32_t> handles{};
    for (const RawObject& object : table) handles.push_back(object.Handle);

    EXPECT_EQ(handles, (std::vector<std::uint32_t>{ 0x0001'0002, 0x0001'0009, 0x0003'0004 }));
    EXPECT_EQ(table.Size(), 3u);
    EXPECT_FALSE(table.IsEmpty());
}

TEST(RawObjectTableTests, ClearAndRefillWithFewerObjectsLeavesNothingBehind)
{
    RawObjectTable table{};
    for (std::uint32_t slot{ 0 }; slot < 10; ++slot) table.Add((1u << 16) | slot, 0, 0, Pattern(100, static_cast<std::uint8_t>(slot)));

    table.Clear();
    EXPECT_TRUE(table.IsEmpty());
    EXPECT_EQ(table.Find((1u << 16) | 3u), nullptr);

    table.Add(0x0005'0003, 0, 0, Pattern(40, 77));
    table.Add(0x0005'0006, 0, 0, Pattern(20, 88));

    EXPECT_EQ(table.Size(), 2u);
    EXPECT_EQ(table.Find((1u << 16) | 3u), nullptr);
    EXPECT_EQ(table.Find((1u << 16) | 7u), nullptr);
    EXPECT_TRUE(Has(table, 0x0005'0003, Pattern(40, 77)));
    EXPECT_TRUE(Has(table, 0x0005'0006, Pattern(20, 88)));
}

TEST(RawObjectTableTests, GrowingTheArenaKeepsEveryObject)
{
    RawObjectTable table{};
    for (std::uint32_t slot{ 0 }; slot < 300; ++slot)
    {
        ASSERT_TRUE(table.Add((1u << 16) | slot, slot, 1, Pattern(3000, static_cast<std::uint8_t>(slot))));
    }

    for (std::uint32_t slot{ 0 }; slot < 300; ++slot) EXPECT_TRUE(Has(table, (1u << 16) | slot, Pattern(3000, static_cast<std::uint8_t>(slot)))) << slot;
}

TEST(RawObjectTableTests, ReadsOfAnObjectStayInsideItsOwnBytes)
{
    RawObjectTable table{};
    table.Add(k_HandleA, 0, 0, Pattern(8, 1));
    table.Add(k_HandleB, 0, 0, Pattern(8, 9));

    const RawObject& first{ *table.Find(k_HandleA) };
    EXPECT_EQ(first.Read<std::uint64_t>(0), 0x1613100D0A070401u);
    EXPECT_EQ(first.Read<std::uint64_t>(1), 0u);
    std::array<std::uint8_t, 4> out{};
    EXPECT_FALSE(first.ReadRaw(6, out.data(), out.size()));
}

TEST(RawObjectTableTests, PopLastTakesBackTheLastObject)
{
    RawObjectTable table{};
    table.Add(k_HandleA, 0, 0, Pattern(8, 1));
    table.Add(k_HandleB, 0, 0, Pattern(8, 2));

    table.PopLast();

    EXPECT_EQ(table.Size(), 1u);
    EXPECT_EQ(table.Find(k_HandleB), nullptr);
    EXPECT_TRUE(table.Add(k_HandleB, 0, 0, Pattern(8, 5)));
    EXPECT_TRUE(Has(table, k_HandleB, Pattern(8, 5)));
}

TEST(RawObjectTableTests, ACopyHasItsOwnBytes)
{
    RawObjectTable table{};
    table.Add(k_HandleA, 0, 0, Pattern(32, 1));
    table.Add(k_HandleB, 0, 0, Pattern(32, 2));

    RawObjectTable copy{ table };
    table.Edit(k_HandleA)[0] ^= 0xFF;

    EXPECT_TRUE(Has(copy, k_HandleA, Pattern(32, 1)));
    EXPECT_FALSE(Has(table, k_HandleA, Pattern(32, 1)));
    EXPECT_NE(copy.Find(k_HandleA)->Bytes.data(), table.Find(k_HandleA)->Bytes.data());

    RawObjectTable assigned{};
    assigned.Add(0x0009'0001, 0, 0, Pattern(8, 4));
    assigned = copy;
    EXPECT_EQ(assigned.Size(), 2u);
    EXPECT_EQ(assigned.Find(0x0009'0001), nullptr);
    EXPECT_TRUE(Has(assigned, k_HandleB, Pattern(32, 2)));
}

TEST(RawObjectTableTests, MovingKeepsTheObjects)
{
    RawObjectTable table{};
    table.Add(k_HandleA, 0, 0, Pattern(32, 1));

    RawObjectTable moved{ std::move(table) };

    EXPECT_TRUE(Has(moved, k_HandleA, Pattern(32, 1)));
}