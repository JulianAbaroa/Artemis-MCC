#include <gtest/gtest.h>

import Common.Log.Type;
import Common.Log.System;
import Blam.Map.Reader.Type;
import Blam.Map.Layer;
import Blam.Map.Tag.Type;
import Blam.MapModel.Layer;
import Blam.MapModel.Definitions.System;
import Tests.Support.Log;
import Tests.Support.Maps;
import std;

// Blam.MapModel (M3, D177-D181): Definitions, World, Vitality and the tag resolver, built from forge_halo.map and shared.map
// as the AIThread does on load. The log lines are the ones the DLL wrote on 2026-10-22 with the code before M3.

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Common::Log::System::Log;
    using Tests::Support::Log::CollectingSink;
    using Tests::Support::Maps::LocatedFile;
    using Tests::Support::Maps::Locate;

    using Blam::Map::Reader::Type::Magic::Tag::k_Coll;
    using Blam::Map::Reader::Type::Magic::Tag::k_Hlmt;
    using Blam::Map::Reader::Type::Magic::Tag::k_Mode;
    using Tests::Support::Maps::k_ForgeHalo;
    using Tests::Support::Maps::k_Shared;

    // The DLL loads the map in about 4 s. The tests only catch a gross slowdown, the report gives the measured time.
    constexpr double k_MaxLoadSeconds{ 30.0 };

    constexpr std::uint16_t k_NoSalt{ 0xFFFF };
    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };

    constexpr std::string_view k_AssaultRifle{ "objects\\weapons\\rifle\\assault_rifle\\assault_rifle" };
    constexpr std::string_view k_Warthog{ "objects\\vehicles\\human\\warthog\\warthog" };
    constexpr std::string_view k_ForgeHaloScenario{ "levels\\multi\\forge_halo\\forge_halo" };

    const std::vector<LogMessage> k_DllLoadLines{
        { .Level = LogLevel::Info, .Tag = "[MapLoaderService]", .Message = "Map loaded." },
        { .Level = LogLevel::Info, .Tag = "[MapBuilderService]",
            .Message = "Raw built. Bipd loaded: 5 | Bloc loaded: 463 | Coll loaded: 560 | Ctrl loaded: 1 | Eqip loaded: 17 |"
                " Hlmt loaded: 590 | Jpt! loaded: 137 | Lbsp loaded: 2 | Mach loaded: 1 | Mode loaded: 599 | Phmo loaded: 562 |"
                " Play loaded: 1 | Proj loaded: 52 | Sbsp loaded: 2 | Scen loaded: 43 | Scnr loaded: 1 | Sddt loaded: 1 |"
                " Sldt loaded: 1 | Vehi loaded: 43 | Weap loaded: 63 | Zone loaded: 1." },
        { .Level = LogLevel::Info, .Tag = "[MapBuilderService]", .Message = "Load completed." },
        { .Level = LogLevel::Info, .Tag = "[DefinitionsBuilder]",
            .Message = "Definitions built. bipd: 5 | bloc: 463 | coll: 560 | ctrl: 1 | eqip: 17 | hlmt: 590 | jpt!: 137 |"
                " mach: 1 | mode: 599 | phmo: 562 | proj: 52 | scen: 43 | scnr: 1 | sddt: 1 | vehi: 43 | weap: 63 | sbsp: 2." },
        { .Level = LogLevel::Info, .Tag = "[DefinitionsBuilder]",
            .Message = "Map limits of levels\\multi\\forge_halo\\forge_halo. Trigger volumes: 33 | Kill triggers: 31 |"
                " Safe zone triggers: 1 | Soft ceilings: 1." },
        { .Level = LogLevel::Info, .Tag = "[WorldBuilder]",
            .Message = "Model links built. Links: 79 | Without coll: 0 | Without mode: 0." },
        { .Level = LogLevel::Info, .Tag = "[WorldBuilder]", .Message = "World built." },
        { .Level = LogLevel::Info, .Tag = "[VitalityBuilder]",
            .Message = "Built. Layouts: 79 | Skipped (no damage sections): 511 | Kill sections: 18 |"
                " Headshot sections: 4 (anchored: 2)." },
    };

    auto CountOf(const CollectingSink& sink, const LogMessage& message) -> std::size_t
    {
        return static_cast<std::size_t>(std::ranges::count(sink.m_Messages, message));
    }

    // return: The four characters of a group magic ("coll"), as the resolver writes them.
    auto FourCC(std::uint32_t magic) -> std::string
    {
        return std::string{ static_cast<char>((magic >> 24) & 0xFF), static_cast<char>((magic >> 16) & 0xFF),
            static_cast<char>((magic >> 8) & 0xFF), static_cast<char>(magic & 0xFF) };
    }

    auto MakeHandle(std::uint16_t salt, std::uint32_t index) -> std::uint32_t
    {
        return (static_cast<std::uint32_t>(salt) << 16) | (index & 0xFFFF);
    }

    // The map and its model, loaded once for the whole suite in the order of AIThread::LoadResources.
    class LoadedMap
    {
    public:
        LoadedMap() = default;
        ~LoadedMap() = default;

        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        Blam::Map::Layer m_Map{ m_Log };
        Blam::MapModel::Layer m_Resolved{ m_Log, m_Map };
        double m_LoadSeconds{};
        std::size_t m_LoadMessages{};
    };
}

class MapModelTests : public ::testing::Test
{
protected:
    static auto SetUpTestSuite() -> void
    {
        const LocatedFile forge{ Locate(k_ForgeHalo) };
        const LocatedFile shared{ Locate(k_Shared) };
        if (forge.Path.empty() || shared.Path.empty())
        {
            s_SkipReason = forge.Path.empty() ? forge.Reason : shared.Reason;
            return;
        }

        s_Loaded = std::make_unique<LoadedMap>();
        auto& loaded = *s_Loaded;
        const auto start = std::chrono::steady_clock::now();

        loaded.m_Map.m_MapLoaderService.OpenFile(shared.Path.string());
        loaded.m_Map.m_MapLoaderService.OpenFile((forge.Path.parent_path() / "campaign.map").string());
        loaded.m_Map.m_MapLoaderService.OpenFile(forge.Path.string());

        loaded.m_Map.m_MapBuilderService.LoadForMap();
        loaded.m_Resolved.m_DefinitionsBuilder.BuildForMap();
        loaded.m_Resolved.m_WorldBuilder.BuildForMap();
        loaded.m_Resolved.m_VitalityBuilder.BuildForMap();

        loaded.m_LoadSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        loaded.m_LoadMessages = loaded.m_Sink.m_Messages.size();
    }

    static auto TearDownTestSuite() -> void
    {
        s_Loaded.reset();
    }

    auto SetUp() -> void override
    {
        if (!s_Loaded) GTEST_SKIP() << s_SkipReason;
    }

    static inline std::unique_ptr<LoadedMap> s_Loaded{};
    static inline std::string s_SkipReason{};
};

TEST_F(MapModelTests, LoadLogsWhatTheDllLogged)
{
    const auto& sink = s_Loaded->m_Sink;
    ASSERT_TRUE(s_Loaded->m_Map.m_FileStore.IsLoaded());

    for (const LogMessage& line : k_DllLoadLines)
    {
        EXPECT_EQ(CountOf(sink, line), 1u) << line.Tag << " " << line.Message;
    }

    const auto load = std::span{ sink.m_Messages }.first(s_Loaded->m_LoadMessages);
    EXPECT_EQ(std::ranges::count(load, LogLevel::Warning, &LogMessage::Level), 0);
    EXPECT_EQ(std::ranges::count(load, LogLevel::Error, &LogMessage::Level), 0);
}

TEST_F(MapModelTests, LoadTimeIsReported)
{
    ::testing::Test::RecordProperty("LoadSeconds", std::format("{:.2f}", s_Loaded->m_LoadSeconds));
    std::cout << std::format("[ MapModel ] forge_halo loaded and built in {:.2f} s.\n", s_Loaded->m_LoadSeconds);
    EXPECT_LT(s_Loaded->m_LoadSeconds, k_MaxLoadSeconds);
}

TEST_F(MapModelTests, KnownTagsHaveDefinitions)
{
    const auto& definitions = s_Loaded->m_Resolved.m_DefinitionsStore;
    EXPECT_TRUE(definitions.m_Weap.Has(std::string{ k_AssaultRifle }));
    EXPECT_TRUE(definitions.m_Vehi.Has(std::string{ k_Warthog }));
    EXPECT_TRUE(definitions.m_Scnr.Has(std::string{ k_ForgeHaloScenario }));
    EXPECT_FALSE(definitions.m_Weap.Has(std::string{ k_Warthog }));
}

TEST_F(MapModelTests, ExplosivesOfForgeHaloReachTheirDamageEffect)
{
    // Measured with B4x: model of the object -> instant response of its damage section -> jpt with the radius and the damage.
    const auto& definitions = s_Loaded->m_Resolved.m_DefinitionsStore;
    const auto* coil{ definitions.m_Hlmt.Get("objects\\gear\\human\\military\\fusion_coil\\fusion_coil") };
    ASSERT_NE(coil, nullptr);
    ASSERT_FALSE(coil->DamageSections.empty());
    ASSERT_FALSE(coil->DamageSections[0].InstantResponses.empty());
    const std::string& blast{ coil->DamageSections[0].InstantResponses.back().TransitionDamageEffectTagName };
    EXPECT_EQ(blast, "objects\\gear\\human\\military\\damage_effects\\fusion_coil_explosion");

    const auto* jpt{ definitions.m_Jpt.Get(blast) };
    ASSERT_NE(jpt, nullptr);
    EXPECT_FLOAT_EQ(jpt->RadiusMin, 0.8f);
    EXPECT_FLOAT_EQ(jpt->RadiusMax, 2.0f);
    EXPECT_FLOAT_EQ(jpt->DamageUpperBoundMax, 200.0f);

    const auto* mine{ definitions.m_Jpt.Get("objects\\multi\\land_mine\\damage_effects\\land_mine_explosion") };
    ASSERT_NE(mine, nullptr);
    EXPECT_FLOAT_EQ(mine->RadiusMin, 0.6f);
    EXPECT_FLOAT_EQ(mine->RadiusMax, 2.0f);
    EXPECT_FLOAT_EQ(mine->DamageUpperBoundMax, 600.0f);
}

TEST(ScnrBuilderTests, MapIdIsCopiedFromTheScnrTag)
{
    Blam::Map::Tag::Type::Scnr::Object::ScnrObject scnr{};
    scnr.Data.MapId = 1234;

    const auto built{ Blam::MapModel::Definitions::System::ScnrBuilder{}.Build(scnr) };
    EXPECT_EQ(built.MapId, 1234);
}

TEST_F(MapModelTests, EveryTagResolvesFromItsHandle)
{
    const auto& index = s_Loaded->m_Map.m_TagIndexStore;
    const auto& resolver = s_Loaded->m_Resolved.m_TagResolverService;
    const std::size_t warningsBefore{ s_Loaded->m_Sink.CountOf(LogLevel::Warning) };

    std::size_t resolved{};
    std::size_t mismatches{};
    const auto tagCount = static_cast<std::int32_t>(index.GetTagsSize());
    for (std::int32_t i = 0; i < tagCount && i < 0xFFFF; ++i)
    {
        const auto& entry = index.GetTag(i);
        if (entry.DatumIndexSalt == k_NoSalt) continue;

        const auto info = resolver.ResolveHandle(MakeHandle(entry.DatumIndexSalt, static_cast<std::uint32_t>(i)));
        const bool hasGroup{ entry.TagGroupIndex >= 0 &&
            static_cast<std::size_t>(entry.TagGroupIndex) < index.GetGroupsSize() };
        const std::string expectedFourCC{ hasGroup ? FourCC(index.GetGroupMagic(entry.TagGroupIndex)) : std::string{} };

        if (!info.IsValid || info.TagName != index.GetTagName(i) || info.FourCC != expectedFourCC) ++mismatches;
        ++resolved;
    }

    EXPECT_GT(resolved, 0u);
    EXPECT_EQ(mismatches, 0u);
    EXPECT_EQ(s_Loaded->m_Sink.CountOf(LogLevel::Warning), warningsBefore);
}

TEST_F(MapModelTests, InvalidHandlesDoNotResolve)
{
    const auto& index = s_Loaded->m_Map.m_TagIndexStore;
    const auto& resolver = s_Loaded->m_Resolved.m_TagResolverService;
    const auto& sink = s_Loaded->m_Sink;

    std::int32_t first{ -1 };
    for (std::int32_t i = 0; i < static_cast<std::int32_t>(index.GetTagsSize()); ++i)
    {
        if (index.GetTag(i).DatumIndexSalt != k_NoSalt)
        {
            first = i;
            break;
        }
    }
    ASSERT_GE(first, 0);

    const std::uint16_t salt{ index.GetTag(first).DatumIndexSalt };
    const std::uint16_t otherSalt{ static_cast<std::uint16_t>(salt == 1 ? 2 : 1) };
    EXPECT_FALSE(resolver.ResolveHandle(MakeHandle(otherSalt, static_cast<std::uint32_t>(first))).IsValid);
    const LogMessage mismatch{ .Level = LogLevel::Warning, .Tag = "[TagResolverService]",
        .Message = std::format("DatumSalt mismatch. Expected: {}, Got: {}.", otherSalt, salt) };
    EXPECT_EQ(CountOf(sink, mismatch), 1u);

    EXPECT_FALSE(resolver.ResolveHandle(k_NoHandle).IsValid);
    const LogMessage invalid{ .Level = LogLevel::Warning, .Tag = "[TagResolverService]",
        .Message = "Salt or Index are invalid." };
    EXPECT_EQ(CountOf(sink, invalid), 1u);

    const auto outOfRange = static_cast<std::uint32_t>(index.GetTagsSize());
    ASSERT_LT(outOfRange, 0xFFFFu);
    EXPECT_FALSE(resolver.ResolveHandle(MakeHandle(salt, outOfRange)).IsValid);
    const LogMessage range{ .Level = LogLevel::Warning, .Tag = "[TagResolverService]",
        .Message = std::format("Index {} is out of range.", outOfRange) };
    EXPECT_EQ(CountOf(sink, range), 1u);
}

TEST_F(MapModelTests, GroupQueriesFollowTheTagIndex)
{
    const auto& index = s_Loaded->m_Map.m_TagIndexStore;
    const auto& resolver = s_Loaded->m_Resolved.m_TagResolverService;

    std::unordered_set<std::string> names{};
    std::unordered_set<std::string> colls{};
    std::unordered_set<std::string> hlmts{};
    std::unordered_set<std::string> modes{};
    for (std::int32_t i = 0; i < static_cast<std::int32_t>(index.GetTagsSize()); ++i)
    {
        const auto& entry = index.GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const std::string name{ index.GetTagName(i) };
        if (name.empty()) continue;

        names.insert(name);
        const std::uint32_t magic{ index.GetGroupMagic(entry.TagGroupIndex) };
        if (magic == k_Coll) colls.insert(name);
        if (magic == k_Hlmt) hlmts.insert(name);
        if (magic == k_Mode) modes.insert(name);
    }
    ASSERT_FALSE(colls.empty());
    ASSERT_FALSE(hlmts.empty());
    ASSERT_FALSE(modes.empty());

    std::size_t mismatches{};
    for (const std::string& name : names)
    {
        if (resolver.HasColl(name) != colls.contains(name)) ++mismatches;
        if (resolver.HasHlmt(name) != hlmts.contains(name)) ++mismatches;
        if (resolver.HasMode(name) != modes.contains(name)) ++mismatches;
    }
    EXPECT_EQ(mismatches, 0u);
    EXPECT_FALSE(resolver.HasColl("objects\\not\\a\\tag"));
}