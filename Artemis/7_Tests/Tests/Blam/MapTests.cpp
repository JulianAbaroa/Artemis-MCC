#include <gtest/gtest.h>

import Common.Log.Type;
import Common.Log.System;
import Map.Reader.Type;
import Map.Layer;
import Tests.Support.Log;
import Tests.Support.Maps;
import std;

// Blam.Map (M3, D177-D181): the reader of the .map file, out of the DLL. The values of forge_halo are the ones the DLL logged
// on 2026-10-22 with the code before M3 (characterization): the move must not change them.

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Common::Log::System::Log;
    using Tests::Support::Log::CollectingSink;
    using Tests::Support::Maps::LocatedFile;
    using Tests::Support::Maps::Locate;

    using Map::Reader::Type::Magic::Tag::k_Scnr;
    using Tests::Support::Maps::k_ForgeHalo;

    // Lowest possible tag count of forge_halo: the sum of the tags the DLL loaded per group.
    constexpr std::size_t k_ForgeHaloLoadedTags{ 3145 };

    constexpr std::string_view k_ForgeHaloScenario{ "levels\\multi\\forge_halo\\forge_halo" };

    const LogMessage k_MapLoaded{ .Level = LogLevel::Info, .Tag = "[MapLoaderService]", .Message = "Map loaded." };
    const LogMessage k_CouldNotOpen{ .Level = LogLevel::Error, .Tag = "[MapLoaderService]",
        .Message = "Could not open map file." };
    const LogMessage k_FailedToLoad{ .Level = LogLevel::Error, .Tag = "[MapLoaderService]", .Message = "Failed to load map." };
    const LogMessage k_LoaderCleanup{ .Level = LogLevel::Info, .Tag = "[MapLoaderService]", .Message = "Cleanup completed." };
    const LogMessage k_BuilderCleanup{ .Level = LogLevel::Info, .Tag = "[MapBuilderService]", .Message = "Cleanup completed." };
    const LogMessage k_LoadCompleted{ .Level = LogLevel::Info, .Tag = "[MapBuilderService]", .Message = "Load completed." };
    const LogMessage k_RawBuilt{ .Level = LogLevel::Info, .Tag = "[MapBuilderService]",
        .Message = "Raw built. Bipd loaded: 5 | Bloc loaded: 463 | Coll loaded: 560 | Ctrl loaded: 1 | Eqip loaded: 17 |"
            " Hlmt loaded: 590 | Jpt! loaded: 137 | Lbsp loaded: 2 | Mach loaded: 1 | Mode loaded: 599 | Phmo loaded: 562 |"
            " Play loaded: 1 | Proj loaded: 52 | Sbsp loaded: 2 | Scen loaded: 43 | Scnr loaded: 1 | Sddt loaded: 1 |"
            " Sldt loaded: 1 | Vehi loaded: 43 | Weap loaded: 63 | Zone loaded: 1." };

    auto CountOf(const CollectingSink& sink, const LogMessage& message) -> std::size_t
    {
        return static_cast<std::size_t>(std::ranges::count(sink.m_Messages, message));
    }

    // Writes the first bytes of a file to a new file of the temp folder, to have a .map that ends too early.
    auto WritePrefix(const std::filesystem::path& source, std::size_t bytes, std::string_view name) -> std::filesystem::path
    {
        std::vector<char> buffer(bytes);
        std::ifstream input{ source, std::ios::binary };
        input.read(buffer.data(), static_cast<std::streamsize>(bytes));

        const std::filesystem::path target{ std::filesystem::temp_directory_path() / name };
        std::ofstream output{ target, std::ios::binary | std::ios::trunc };
        output.write(buffer.data(), input.gcount());
        return target;
    }

    // Opens the files of a multiplayer map the way the engine reports them: the two cache files and then the map.
    auto OpenWithCacheFiles(Map::Layer& map, const std::filesystem::path& mapPath) -> void
    {
        const std::filesystem::path folder{ mapPath.parent_path() };
        map.m_MapLoaderService.OpenFile((folder / "shared.map").string());
        map.m_MapLoaderService.OpenFile((folder / "campaign.map").string());
        map.m_MapLoaderService.OpenFile(mapPath.string());
    }
}

TEST(MapLoaderTests, CacheFilesOnlyRecordTheirPath)
{
    CollectingSink sink{};
    Log log{ sink };
    Map::Layer map{ log };

    const std::string shared{ "Z:\\NoGame\\haloreach\\maps\\shared.map" };
    const std::string campaign{ "Z:\\NoGame\\haloreach\\maps\\campaign.map" };
    map.m_MapLoaderService.OpenFile(shared);
    map.m_MapLoaderService.OpenFile(campaign);

    EXPECT_EQ(map.m_FileStore.GetSharedFilePath(), shared);
    EXPECT_EQ(map.m_FileStore.GetCampaignFilePath(), campaign);
    EXPECT_EQ(map.m_FileStore.GetMapFilePath(), "");
    EXPECT_FALSE(map.m_FileStore.IsLoaded());
    EXPECT_TRUE(sink.m_Messages.empty());
}

TEST(MapLoaderTests, OnlyTheFileNameDecidesWhatWasOpened)
{
    CollectingSink sink{};
    Log log{ sink };
    Map::Layer map{ log };

    // Folders named like the cache files do not count (D179): this is a map, and it does not exist.
    const std::string path{ "Z:\\shared\\campaign\\haloreach\\maps\\forge_halo.map" };
    map.m_MapLoaderService.OpenFile(path);

    EXPECT_EQ(map.m_FileStore.GetMapFilePath(), path);
    EXPECT_EQ(map.m_FileStore.GetSharedFilePath(), "");
    EXPECT_EQ(map.m_FileStore.GetCampaignFilePath(), "");
    EXPECT_FALSE(map.m_FileStore.IsLoaded());
    EXPECT_EQ(CountOf(sink, k_CouldNotOpen), 1u);
    EXPECT_EQ(sink.m_Messages.size(), 1u);
}

TEST(MapLoaderTests, MapThatEndsTooEarlyIsNotLoaded)
{
    const LocatedFile forge{ Locate(k_ForgeHalo) };
    if (forge.Path.empty()) GTEST_SKIP() << forge.Reason;

    // 16 bytes: the header is cut. 64 KiB: the header is whole, the tag index is past the end.
    for (const std::size_t bytes : { std::size_t{ 16 }, std::size_t{ 64 * 1024 } })
    {
        SCOPED_TRACE(bytes);
        const std::filesystem::path cut{ WritePrefix(forge.Path, bytes, std::format("artemis_m3_cut_{}.map", bytes)) };

        CollectingSink sink{};
        Log log{ sink };
        Map::Layer map{ log };
        map.m_MapLoaderService.OpenFile(cut.string());

        EXPECT_FALSE(map.m_FileStore.IsLoaded());
        EXPECT_EQ(CountOf(sink, k_FailedToLoad), 1u);
        EXPECT_EQ(CountOf(sink, k_MapLoaded), 0u);

        std::error_code error{};
        std::filesystem::remove(cut, error);
    }
}

TEST(MapLoaderTests, ForgeHaloLoadsItsTagIndex)
{
    const LocatedFile forge{ Locate(k_ForgeHalo) };
    if (forge.Path.empty()) GTEST_SKIP() << forge.Reason;

    CollectingSink sink{};
    Log log{ sink };
    Map::Layer map{ log };
    OpenWithCacheFiles(map, forge.Path);

    ASSERT_TRUE(map.m_FileStore.IsLoaded());
    EXPECT_EQ(map.m_FileStore.GetMapFilePath(), forge.Path.string());
    EXPECT_EQ(CountOf(sink, k_MapLoaded), 1u);
    EXPECT_GE(map.m_TagIndexStore.GetTagsSize(), k_ForgeHaloLoadedTags);

    // The scenario of the map is in the index, in the scnr group.
    bool hasScenario{ false };
    const auto tagCount = static_cast<std::int32_t>(map.m_TagIndexStore.GetTagsSize());
    for (std::int32_t i = 0; i < tagCount; ++i)
    {
        if (map.m_TagIndexStore.GetTagName(i) != k_ForgeHaloScenario) continue;

        const std::int16_t group{ map.m_TagIndexStore.GetTag(i).TagGroupIndex };
        if (map.m_TagIndexStore.GetGroupMagic(group) == k_Scnr) hasScenario = true;
    }
    EXPECT_TRUE(hasScenario);
}

TEST(MapLoaderTests, ForgeHaloRawTagsMatchTheDll)
{
    const LocatedFile forge{ Locate(k_ForgeHalo) };
    if (forge.Path.empty()) GTEST_SKIP() << forge.Reason;

    CollectingSink sink{};
    Log log{ sink };
    Map::Layer map{ log };
    OpenWithCacheFiles(map, forge.Path);
    ASSERT_TRUE(map.m_FileStore.IsLoaded());

    map.m_MapBuilderService.LoadForMap();

    EXPECT_EQ(CountOf(sink, k_RawBuilt), 1u);
    EXPECT_EQ(CountOf(sink, k_LoadCompleted), 1u);
    EXPECT_EQ(sink.CountOf(LogLevel::Warning), 0u);
    EXPECT_EQ(sink.CountOf(LogLevel::Error), 0u);
}

TEST(MapLoaderTests, CleanupForgetsTheMap)
{
    const LocatedFile forge{ Locate(k_ForgeHalo) };
    if (forge.Path.empty()) GTEST_SKIP() << forge.Reason;

    CollectingSink sink{};
    Log log{ sink };
    Map::Layer map{ log };
    OpenWithCacheFiles(map, forge.Path);
    ASSERT_TRUE(map.m_FileStore.IsLoaded());

    map.Cleanup();

    EXPECT_FALSE(map.m_FileStore.IsLoaded());
    EXPECT_EQ(map.m_FileStore.GetMapFilePath(), "");
    EXPECT_EQ(map.m_FileStore.GetSharedFilePath(), "");
    EXPECT_EQ(map.m_TagIndexStore.GetTagsSize(), 0u);
    EXPECT_EQ(CountOf(sink, k_LoaderCleanup), 1u);
    EXPECT_EQ(CountOf(sink, k_BuilderCleanup), 1u);
}