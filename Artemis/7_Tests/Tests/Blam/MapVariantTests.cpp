#include <gtest/gtest.h>

import Blam.Layout.Object.Type;
import std;

// The pure logic of the map variants (Q98, D266): header read, compatibility check and runtime composition, on synthetic blocks.

namespace
{
    using Blam::Layout::Object::Type::MapVariant::CheckCompatible;
    using Blam::Layout::Object::Type::MapVariant::ComposeRuntime;
    using Blam::Layout::Object::Type::MapVariant::MapVariantHeader;
    using Blam::Layout::Object::Type::MapVariant::Read;

    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantMapIdOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantQuotasOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersionOffset;
    using Blam::Layout::Object::Type::Constant::k_MaxPlacements;
    using Blam::Layout::Object::Type::Constant::k_PlacementSize;
    using Blam::Layout::Object::Type::Constant::k_PlacementsOffset;

    using Block = std::vector<std::byte>;

    template <typename T>
    auto Put(Block& block, std::size_t offset, T value) -> void
    {
        std::memcpy(block.data() + offset, &value, sizeof(T));
    }

    // return: A map variant block: header fields as given, placements in use for the first count slots, bytes of the rest set to fill.
    auto MakeBlock(std::uint16_t version, std::uint16_t quotas, std::uint32_t mapId, std::uint32_t placements, std::uint8_t fill) -> Block
    {
        Block block(k_MapVariantSize, static_cast<std::byte>(fill));
        Put<std::uint16_t>(block, k_MapVariantVersionOffset, version);
        Put<std::uint16_t>(block, k_MapVariantQuotasOffset, quotas);
        Put<std::uint32_t>(block, k_MapVariantMapIdOffset, mapId);

        for (std::uint32_t index{ 0 }; index < k_MaxPlacements; ++index)
        {
            Put<std::uint16_t>(block, k_PlacementsOffset + index * k_PlacementSize, index < placements ? 0x1 : 0x0);
        }

        return block;
    }
}

TEST(MapVariantTests, ReadGivesTheHeader)
{
    const Block block{ MakeBlock(k_MapVariantVersion, 145, 3006, 462, 0x11) };

    const auto header{ Read(block) };

    ASSERT_TRUE(header.has_value());
    EXPECT_EQ(header->Version, k_MapVariantVersion);
    EXPECT_EQ(header->Quotas, 145);
    EXPECT_EQ(header->MapId, 3006u);
    EXPECT_EQ(header->PlacementsInUse, 462u);
}

TEST(MapVariantTests, ReadRejectsAShortBlock)
{
    const Block block(k_MapVariantSize - 1, std::byte{ 0 });

    EXPECT_FALSE(Read(block).has_value());
    EXPECT_FALSE(Read({}).has_value());
}

TEST(MapVariantTests, ReadCountsOnlyTheInUseFlag)
{
    Block block{ MakeBlock(k_MapVariantVersion, 145, 3006, 0, 0) };
    Put<std::uint16_t>(block, k_PlacementsOffset, 0xFFFE);
    Put<std::uint16_t>(block, k_PlacementsOffset + k_PlacementSize, 0x0003);

    EXPECT_EQ(Read(block)->PlacementsInUse, 1u);
}

TEST(MapVariantTests, CompatibleWhenVersionMapAndQuotasMatch)
{
    const MapVariantHeader current{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006, .PlacementsInUse = 462 };
    const MapVariantHeader incoming{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006, .PlacementsInUse = 651 };

    EXPECT_FALSE(CheckCompatible(incoming, current).has_value());
}

TEST(MapVariantTests, DifferentMapIdGivesTheReason)
{
    const MapVariantHeader current{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006 };
    MapVariantHeader incoming{ current };
    incoming.MapId = 3010;

    const auto reason{ CheckCompatible(incoming, current) };

    ASSERT_TRUE(reason.has_value());
    EXPECT_NE(reason->find("map id 3010 instead of 3006"), std::string::npos);
}

TEST(MapVariantTests, WrongVersionGivesTheReason)
{
    const MapVariantHeader current{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006 };
    MapVariantHeader incoming{ current };
    incoming.Version = 0x1F;

    const auto reason{ CheckCompatible(incoming, current) };

    ASSERT_TRUE(reason.has_value());
    EXPECT_NE(reason->find("version 0x1F instead of 0x20"), std::string::npos);
}

TEST(MapVariantTests, DifferentQuotasGiveTheReason)
{
    const MapVariantHeader current{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006 };
    MapVariantHeader incoming{ current };
    incoming.Quotas = 120;

    const auto reason{ CheckCompatible(incoming, current) };

    ASSERT_TRUE(reason.has_value());
    EXPECT_NE(reason->find("120 quotas instead of 145"), std::string::npos);
}

TEST(MapVariantTests, ComposeRuntimeKeepsOnlyTheWorldBounds)
{
    const Block incoming{ MakeBlock(k_MapVariantVersion, 145, 3006, 651, 0xAA) };
    const Block current{ MakeBlock(k_MapVariantVersion, 145, 3006, 462, 0xBB) };

    const Block runtime{ ComposeRuntime(incoming, current) };

    ASSERT_EQ(runtime.size(), k_MapVariantSize);

    for (std::size_t offset{ 0 }; offset < k_MapVariantSize; ++offset)
    {
        const bool isBounds{ offset >= k_MapVariantBoundsOffset && offset < k_MapVariantBoundsOffset + k_MapVariantBoundsSize };
        const Block& source{ isBounds ? current : incoming };

        ASSERT_EQ(runtime[offset], source[offset]) << "offset 0x" << std::hex << offset;
    }

    EXPECT_EQ(Read(runtime)->PlacementsInUse, 651u);
}

TEST(MapVariantTests, ComposeRuntimeRejectsAShortBlock)
{
    const Block full{ MakeBlock(k_MapVariantVersion, 145, 3006, 1, 0) };
    const Block shortBlock(k_MapVariantSize - 1, std::byte{ 0 });

    EXPECT_TRUE(ComposeRuntime(shortBlock, full).empty());
    EXPECT_TRUE(ComposeRuntime(full, shortBlock).empty());
}