#include <gtest/gtest.h>
#include <process.h>

import Common.Log.System;
import Common.Log.Type;
import Engine.Match.Type;
import Engine.Match.State;
import Engine.Match.System;
import Tests.Support.Log;
import std;

// MapVariantCatalogService and MapVariantDecodeStore (Q106): the listing of the .mvar files of every account folder by the name inside them,
// and the request and result of a decode. The folder is a temporary copy of the layout of MCC, with the two real files of the fixtures.

namespace
{
    using Common::Log::System::Log;
    using Engine::Match::State::MapVariantDecodeStore;
    using Engine::Match::System::MapVariantCatalogService;
    using Engine::Match::Type::MapVariantDecodeOutcome;
    using Tests::Support::Log::CollectingSink;

    using std::filesystem::path;

    // This file is Artemis/7_Tests/Tests/Engine/MapVariantCatalogTests.cpp.
    auto FixtureFolder() -> path
    {
        return path{ std::source_location::current().file_name() }.parent_path().parent_path() / "Blam" / "Fixtures" / "mvar";
    }

    class MapVariantCatalogTests : public testing::Test
    {
    protected:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        MapVariantCatalogService m_Service{ m_Log };
        path m_Folder{};

        auto SetUp() -> void override
        {
            const std::string test{ testing::UnitTest::GetInstance()->current_test_info()->name() };
            m_Folder = std::filesystem::temp_directory_path() / std::format("artemis_tests_{}_{}", _getpid(), test);

            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
            std::filesystem::create_directories(m_Folder, error);
        }

        auto TearDown() -> void override
        {
            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
        }

        // Puts a copy of a fixture in the map folder of an account.
        auto Place(std::string_view account, std::string_view fixture, std::string_view fileName) const -> path
        {
            const path folder{ m_Folder / account / "HaloReach" / "Map" };
            std::error_code error{};
            std::filesystem::create_directories(folder, error);
            std::filesystem::copy_file(FixtureFolder() / fixture, folder / fileName, error);
            EXPECT_FALSE(error) << error.message();
            return folder / fileName;
        }
    };
}

TEST_F(MapVariantCatalogTests, ListsEveryAccountSortedByName)
{
    this->Place("0002", "ctf_2v2_final.mvar", "bbb.mvar");
    this->Place("0001", "ctf_1v1_first.mvar", "aaa.mvar");
    this->Place("0001", "ctf_2v2_final.mvar", "ccc.mvar");

    const auto entries{ m_Service.ListIn(m_Folder) };

    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].Name, "CTF (1v1) First");
    EXPECT_EQ(entries[0].Account, "0001");
    EXPECT_EQ(entries[0].Path.filename(), "aaa.mvar");
    EXPECT_EQ(entries[1].Name, "CTF (2v2) Final");
    EXPECT_EQ(entries[1].Account, "0001");
    EXPECT_EQ(entries[2].Account, "0002");
    EXPECT_EQ(entries[0].Description, "AI training");
}

TEST_F(MapVariantCatalogTests, SkipsFilesThatAreNotMapVariantsAndOtherExtensions)
{
    this->Place("0001", "ctf_1v1_first.mvar", "good.mvar");
    const path folder{ m_Folder / "0001" / "HaloReach" / "Map" };

    std::ofstream{ folder / "broken.mvar", std::ios::binary } << "not a blf file at all";
    std::ofstream{ folder / "notes.txt" } << "hello";

    const auto entries{ m_Service.ListIn(m_Folder) };

    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].Path.filename(), "good.mvar");
}

TEST_F(MapVariantCatalogTests, AFileWithoutNameShowsItsFileName)
{
    const path file{ this->Place("0001", "ctf_1v1_first.mvar", "noname.mvar") };

    // The name is the UTF-16 field at +0xC0 of the file: clear it.
    std::fstream stream{ file, std::ios::in | std::ios::out | std::ios::binary };
    stream.seekp(0xC0);
    const std::string zeros(0x100, '\0');
    stream.write(zeros.data(), static_cast<std::streamsize>(zeros.size()));
    stream.close();

    const auto entries{ m_Service.ListIn(m_Folder) };

    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].Name, "noname.mvar");
}

TEST_F(MapVariantCatalogTests, AMissingFolderGivesAnEmptyList)
{
    EXPECT_TRUE(m_Service.ListIn(m_Folder / "nothing").empty());
}

TEST(MapVariantDecodeStoreTests, ARequestIsTakenOnceAndItsResultIsGivenOnce)
{
    MapVariantDecodeStore store{};
    EXPECT_FALSE(store.HasRequest());

    const std::uint64_t ticket{ store.Request(std::vector<std::byte>(4, std::byte{ 1 })) };

    EXPECT_TRUE(store.HasRequest());
    EXPECT_FALSE(store.TakeOutcome(ticket).has_value());

    const auto request{ store.TakeRequest() };
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->Ticket, ticket);
    EXPECT_EQ(request->File.size(), 4u);
    EXPECT_FALSE(store.HasRequest());
    EXPECT_FALSE(store.TakeRequest().has_value());

    MapVariantDecodeOutcome outcome{ .Ticket = ticket };
    outcome.Decoded.Name = "Decoded";
    store.Publish(std::move(outcome));

    const auto result{ store.TakeOutcome(ticket) };
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->Decoded.Name, "Decoded");
    EXPECT_FALSE(store.TakeOutcome(ticket).has_value());
}

TEST(MapVariantDecodeStoreTests, ANewRequestReplacesTheOldOneAndItsStaleResultIsDropped)
{
    MapVariantDecodeStore store{};
    const std::uint64_t first{ store.Request({}) };
    const std::uint64_t second{ store.Request({}) };

    EXPECT_NE(first, second);

    store.Publish(MapVariantDecodeOutcome{ .Ticket = first });

    EXPECT_FALSE(store.TakeOutcome(first).has_value());
    EXPECT_FALSE(store.TakeOutcome(second).has_value());
    EXPECT_EQ(store.TakeRequest()->Ticket, second);
}