#include <gtest/gtest.h>

import Common.Log.Type;
import Common.Log.System;
import Map.Reader.Type;
import Map.Reader.State;
import Resolved.Tag.System;
import Tests.Support.Blam;
import Tests.Support.Log;
import std;

// The tag resolver of Blam.MapModel (M3, D180) on a tag index written by hand: no .map needed.

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Common::Log::System::Log;
    using Map::Reader::State::FileStore;
    using Map::Reader::State::TagIndexStore;
    using Resolved::Tag::System::TagResolverService;
    using Tests::Support::Blam::SyntheticTag;
    using Tests::Support::Blam::Fill;
    using Tests::Support::Blam::MakeHandle;
    using Tests::Support::Log::CollectingSink;

    using Map::Reader::Type::Magic::Tag::k_Coll;
    using Map::Reader::Type::Magic::Tag::k_Hlmt;
    using Map::Reader::Type::Magic::Tag::k_Mode;

    auto CountOf(const CollectingSink& sink, std::string_view message) -> std::size_t
    {
        const LogMessage expected{ .Level = LogLevel::Warning, .Tag = "[TagResolverService]", .Message = std::string{ message } };
        return static_cast<std::size_t>(std::ranges::count(sink.m_Messages, expected));
    }

    const std::array<std::uint32_t, 2> k_Groups{ k_Coll, k_Mode };

    // 0 and 1 share a name in two groups, 2 has no group, 3 has no name, 4 has an invalid salt.
    const std::array<SyntheticTag, 5> k_Tags{ {
        { .Group = 0, .Salt = 0xE000, .Name = "objects\\crate\\crate" },
        { .Group = 1, .Salt = 0xE001, .Name = "objects\\crate\\crate" },
        { .Group = -1, .Salt = 0xE002, .Name = "objects\\loose\\loose" },
        { .Group = 0, .Salt = 0xE003, .Name = "" },
        { .Group = 0, .Salt = 0xFFFF, .Name = "objects\\gone\\gone" },
    } };

    // A resolver over the hand written index, with the map marked as loaded.
    class TagResolverTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            Fill(m_Index, k_Groups, k_Tags);
            m_Files.SetLoaded(true);
        }

        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        FileStore m_Files{};
        TagIndexStore m_Index{};
        TagResolverService m_Resolver{ m_Log, m_Files, m_Index };
    };
}

TEST(TagResolverNoMapTests, NothingResolvesBeforeAMapIsLoaded)
{
    CollectingSink sink{};
    Log log{ sink };
    FileStore files{};
    TagIndexStore index{};
    TagResolverService resolver{ log, files, index };

    EXPECT_FALSE(resolver.ResolveHandle(MakeHandle(0xE000, 0)).IsValid);
    EXPECT_EQ(CountOf(sink, "No map loaded."), 1u);
}

TEST_F(TagResolverTests, HandleGivesNameAndGroup)
{
    const auto coll = m_Resolver.ResolveHandle(MakeHandle(0xE000, 0));
    EXPECT_TRUE(coll.IsValid);
    EXPECT_EQ(coll.FourCC, "coll");
    EXPECT_EQ(coll.TagName, "objects\\crate\\crate");

    const auto mode = m_Resolver.ResolveHandle(MakeHandle(0xE001, 1));
    EXPECT_TRUE(mode.IsValid);
    EXPECT_EQ(mode.FourCC, "mode");

    EXPECT_TRUE(m_Sink.m_Messages.empty());
}

TEST_F(TagResolverTests, TagWithoutGroupOrNameStillResolves)
{
    const auto loose = m_Resolver.ResolveHandle(MakeHandle(0xE002, 2));
    EXPECT_TRUE(loose.IsValid);
    EXPECT_EQ(loose.FourCC, "");
    EXPECT_EQ(loose.TagName, "objects\\loose\\loose");

    const auto unnamed = m_Resolver.ResolveHandle(MakeHandle(0xE003, 3));
    EXPECT_TRUE(unnamed.IsValid);
    EXPECT_EQ(unnamed.TagName, "unknown");
}

TEST_F(TagResolverTests, InvalidHandlesAreRejectedWithAWarning)
{
    EXPECT_FALSE(m_Resolver.ResolveHandle(0xFFFFFFFF).IsValid);
    EXPECT_EQ(CountOf(m_Sink, "Salt or Index are invalid."), 1u);

    EXPECT_FALSE(m_Resolver.ResolveHandle(MakeHandle(0xE000, 7)).IsValid);
    EXPECT_EQ(CountOf(m_Sink, "Index 7 is out of range."), 1u);

    EXPECT_FALSE(m_Resolver.ResolveHandle(MakeHandle(0xE004, 4)).IsValid);
    EXPECT_EQ(CountOf(m_Sink, "DatumSalt is invalid."), 1u);

    EXPECT_FALSE(m_Resolver.ResolveHandle(MakeHandle(0xE001, 0)).IsValid);
    EXPECT_EQ(CountOf(m_Sink, "DatumSalt mismatch. Expected: 57345, Got: 57344."), 1u);
}

TEST_F(TagResolverTests, GroupQueriesUseEveryGroupOfAName)
{
    EXPECT_TRUE(m_Resolver.HasColl("objects\\crate\\crate"));
    EXPECT_TRUE(m_Resolver.HasMode("objects\\crate\\crate"));
    EXPECT_FALSE(m_Resolver.HasHlmt("objects\\crate\\crate"));

    // Without a group, or not in the index at all.
    EXPECT_FALSE(m_Resolver.HasColl("objects\\loose\\loose"));
    EXPECT_FALSE(m_Resolver.HasColl("objects\\missing\\missing"));
}

TEST_F(TagResolverTests, CleanupIndexesTheGroupsAgain)
{
    EXPECT_FALSE(m_Resolver.HasHlmt("objects\\crate\\crate"));

    // The groups of each name are indexed on the first query and kept until Cleanup (one map at a time).
    m_Index.GetGroupsData()[1].Magic = static_cast<std::int32_t>(k_Hlmt);
    EXPECT_FALSE(m_Resolver.HasHlmt("objects\\crate\\crate"));

    m_Resolver.Cleanup();
    EXPECT_TRUE(m_Resolver.HasHlmt("objects\\crate\\crate"));
    EXPECT_FALSE(m_Resolver.HasMode("objects\\crate\\crate"));
}