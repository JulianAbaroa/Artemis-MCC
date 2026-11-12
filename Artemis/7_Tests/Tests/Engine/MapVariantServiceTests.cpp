#include <gtest/gtest.h>
#include <process.h>

import Common.Log.System;
import Common.Log.Type;
import Services.Settings.State;
import Engine.Base.MapVariant.Type;
import Engine.Base.MapVariant.State;
import Engine.Base.MapVariant.System;
import Engine.Match.Type;
import Engine.Match.State;
import Engine.Match.System;
import Blam.Layout.Object.Type;
import Tests.Support.Log;
import std;

// MapVariantService and MapVariantStore (Q98, D266): the map variant of a recorded configuration, active for the next start and written
// into the configuration the engine starts with. The service works on a temporary storage folder and the configuration of MCC is a buffer
// built by the test.

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Engine::Base::MapVariant::State::MapVariantStore;
    using Engine::Base::MapVariant::Type::ActiveMapVariant;
    using Engine::Match::State::MatchConfigStore;
    using Engine::Base::MapVariant::System::MapVariantService;
    using Engine::Match::System::MatchConfigService;
    using Services::Settings::State::SettingsStore;
    using Tests::Support::Log::CollectingSink;

    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelCountOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelOffsetsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelStringsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantMapIdOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantQuotasOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersionOffset;
    using Engine::Match::Type::k_EngineConfigurationSize;
    using Engine::Match::Type::k_TransplantBlocks;

    using std::filesystem::path;

    using Buffer = std::vector<std::byte>;

    class MapVariantServiceTests : public testing::Test
    {
    public:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        SettingsStore m_SettingsStore{};
        MatchConfigStore m_ConfigStore{};
        MatchConfigService m_ConfigService{ m_Log, m_SettingsStore, m_ConfigStore };
        MapVariantStore m_Store{};
        MapVariantService m_Service{ m_Log, m_Store };
        path m_Folder{};

        auto SetUp() -> void override
        {
            const std::string test{ testing::UnitTest::GetInstance()->current_test_info()->name() };
            m_Folder = std::filesystem::temp_directory_path() / std::format("artemis_tests_mv_{}_{}", _getpid(), test);

            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
            std::filesystem::create_directories(m_Folder, error);

            m_SettingsStore.SetAppDataDirectory(m_Folder.string());
        }

        auto TearDown() -> void override
        {
            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
        }

        // return: A configuration of k_EngineConfigurationSize bytes set to fill, with the header of a map variant at its place.
        static auto MakeConfig(std::uint8_t fill, std::uint16_t quotas, std::uint32_t mapId, std::uint8_t mapVariantFill) -> Buffer
        {
            Buffer config(k_EngineConfigurationSize, static_cast<std::byte>(fill));
            std::memset(config.data() + k_ConfigurationMapVariantOffset, mapVariantFill, k_MapVariantSize);

            const std::uint16_t version{ k_MapVariantVersion };
            std::memcpy(config.data() + k_ConfigurationMapVariantOffset + k_MapVariantVersionOffset, &version, sizeof(version));
            std::memcpy(config.data() + k_ConfigurationMapVariantOffset + k_MapVariantQuotasOffset, &quotas, sizeof(quotas));
            std::memcpy(config.data() + k_ConfigurationMapVariantOffset + k_MapVariantMapIdOffset, &mapId, sizeof(mapId));

            // No labels: the table is read when a map variant is written between rounds, and a filler would be a count out of range.
            const std::int32_t labels{ 0 };
            std::memcpy(config.data() + k_ConfigurationMapVariantOffset + k_MapVariantLabelCountOffset, &labels, sizeof(labels));
            return config;
        }

        auto WriteConfig(const std::string& name, const Buffer& bytes) const -> void
        {
            const path library{ m_Folder / "MatchConfigs" };
            std::filesystem::create_directories(library);
            std::ofstream stream{ library / (name + ".bin"), std::ios::binary | std::ios::trunc };
            stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }

        // return: The map variant of a recorded configuration, read as the bench and the bridge do before they call the service.
        auto Read(const std::string& name) const -> Buffer
        {
            return m_ConfigService.ReadMapVariant(name);
        }

        // return: The map variant of a recorded configuration, resolved as the bench and the bridge do.
        auto Pick(const std::string& name) const -> std::shared_ptr<const ActiveMapVariant>
        {
            auto pick{ m_Service.Resolve(name, this->Read(name)) };
            EXPECT_TRUE(pick.has_value()) << name;
            return pick.value_or(nullptr);
        }

        auto Logged(LogLevel level, std::string_view text) const -> bool
        {
            return std::ranges::any_of(m_Sink.m_Messages, [&](const LogMessage& message) {
                return message.Level == level && message.Message.find(text) != std::string::npos;
                });
        }

        // return: Whether the two buffers differ only inside the map variant.
        static auto DifferOnlyInMapVariant(const Buffer& a, const Buffer& b) -> bool
        {
            for (std::size_t offset{ 0 }; offset < a.size(); ++offset)
            {
                const bool isInside{ offset >= k_ConfigurationMapVariantOffset && offset < k_ConfigurationMapVariantOffset + k_MapVariantSize };
                if (!isInside && a[offset] != b[offset]) return false;
            }

            return true;
        }
    };
}

TEST_F(MapVariantServiceTests, ResolveReadsTheMapVariantOfTheConfigAndChangesNothing)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));

    const auto pick{ m_Service.Resolve("reach bus", this->Read("reach bus")) };

    ASSERT_TRUE(pick.has_value());
    ASSERT_NE(*pick, nullptr);
    EXPECT_EQ((*pick)->Name, "reach bus");
    EXPECT_EQ((*pick)->Block.size(), k_MapVariantSize);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
    EXPECT_TRUE(m_Service.GetActiveName().empty());
}

TEST_F(MapVariantServiceTests, ResolveOfNoNameIsANullPickThatMeansTheMapOfTheLoad)
{
    const auto pick{ m_Service.Resolve("", {}) };

    ASSERT_TRUE(pick.has_value());
    EXPECT_EQ(*pick, nullptr);
}

TEST_F(MapVariantServiceTests, ResolveOfAMissingConfigFailsWithAReasonAndLogsAnError)
{
    const auto pick{ m_Service.Resolve("missing", this->Read("missing")) };

    ASSERT_FALSE(pick.has_value());
    EXPECT_NE(pick.error().find("missing"), std::string::npos);
    EXPECT_TRUE(Logged(LogLevel::Error, "could not be used"));
}

TEST_F(MapVariantServiceTests, AdoptMakesThePickTheActiveOneAndNullClearsIt)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));

    m_Service.Adopt(this->Pick("reach bus"));

    EXPECT_EQ(m_Service.GetActiveName(), "reach bus");
    ASSERT_NE(m_Service.GetActive(), nullptr);

    m_Service.Adopt(nullptr);
    EXPECT_EQ(m_Service.GetActive(), nullptr);
    EXPECT_TRUE(m_Service.GetActiveName().empty());
}

TEST_F(MapVariantServiceTests, WriteToStartWritesTheBlockAndNothingOutsideIt)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    const auto pick{ this->Pick("reach bus") };

    const Buffer original{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{ original };

    EXPECT_TRUE(m_Service.WriteToStart(start, *pick).has_value());

    EXPECT_TRUE(DifferOnlyInMapVariant(start, original));
    // Adopting is the business of the setup service once everything is in.
    EXPECT_EQ(m_Store.GetActive(), nullptr);

    const Buffer expected{ MakeConfig(0x11, 145, 3006, 0x5C) };
    EXPECT_TRUE(std::equal(start.begin() + k_ConfigurationMapVariantOffset, start.begin() + k_ConfigurationMapVariantOffset + k_MapVariantSize,
        expected.begin() + k_ConfigurationMapVariantOffset));
}

TEST_F(MapVariantServiceTests, WriteToStartRejectsAnotherMapAndLeavesTheBufferEqual)
{
    this->WriteConfig("other map", MakeConfig(0x11, 145, 3010, 0x5C));
    const auto pick{ this->Pick("other map") };

    const Buffer original{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{ original };

    const auto written{ m_Service.WriteToStart(start, *pick) };

    ASSERT_FALSE(written.has_value());
    EXPECT_EQ(start, original);
    EXPECT_NE(written.error().find("map id 3010 instead of 3006"), std::string::npos);
}

TEST_F(MapVariantServiceTests, WriteToStartRejectsAnotherNumberOfQuotas)
{
    this->WriteConfig("other quotas", MakeConfig(0x11, 120, 3006, 0x5C));
    const auto pick{ this->Pick("other quotas") };

    const Buffer original{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{ original };

    const auto written{ m_Service.WriteToStart(start, *pick) };

    ASSERT_FALSE(written.has_value());
    EXPECT_EQ(start, original);
    EXPECT_NE(written.error().find("120 quotas instead of 145"), std::string::npos);
}

TEST_F(MapVariantServiceTests, WriteToStartRefusesABufferTooSmall)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    const auto pick{ this->Pick("reach bus") };

    Buffer start(k_ConfigurationMapVariantOffset + k_MapVariantSize - 1, std::byte{ 0x77 });
    const Buffer original{ start };

    EXPECT_FALSE(m_Service.WriteToStart(start, *pick).has_value());
    EXPECT_EQ(start, original);
}

TEST_F(MapVariantServiceTests, TheMapVariantGoesOverTheTransplantedOneAndTheRestOfTheTransplantStays)
{
    // The order of the start: transplant, then the map variant of the setup (D266).
    this->WriteConfig("recorded", MakeConfig(0x22, 145, 3006, 0x33));
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_ConfigService.LoadPending("recorded"));
    const auto pick{ this->Pick("reach bus") };

    const Buffer mcc{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{};
    ASSERT_TRUE(m_ConfigService.Transplant(mcc.data(), start));

    const Buffer transplanted{ start };
    EXPECT_EQ(static_cast<std::uint8_t>(transplanted[k_ConfigurationMapVariantOffset + 0x10]), 0x33);

    EXPECT_TRUE(m_Service.WriteToStart(start, *pick).has_value());

    EXPECT_TRUE(DifferOnlyInMapVariant(start, transplanted));
    EXPECT_EQ(static_cast<std::uint8_t>(start[k_ConfigurationMapVariantOffset + 0x10]), 0x5C);
}

TEST_F(MapVariantServiceTests, TheMapVariantDoesNotOverlapTheGameVariantTheNextStepWrites)
{
    // The preset writes inside the game variant, the third transplant block, after the map variant is in.
    constexpr std::size_t k_VariantEnd{ k_TransplantBlocks[2].Offset + k_TransplantBlocks[2].Size };

    EXPECT_LE(k_VariantEnd, k_ConfigurationMapVariantOffset);
    EXPECT_LE(k_ConfigurationMapVariantOffset + k_MapVariantSize, k_EngineConfigurationSize);
    EXPECT_GE(k_ConfigurationMapVariantOffset, k_TransplantBlocks.back().Offset);
}

// --- The live map variant (round break) --------------------------------------------------------------------------------------------

namespace
{
    // return: A live map variant (runtime or options) of k_MapVariantSize bytes with the header of a map variant, the bounds set to bounds
    // and every other byte set to fill.
    auto MakeLive(std::uint8_t fill, std::uint16_t quotas, std::uint32_t mapId, std::uint8_t bounds) -> Buffer
    {
        Buffer live{ MapVariantServiceTests::MakeConfig(0, quotas, mapId, fill) };
        Buffer block(live.begin() + k_ConfigurationMapVariantOffset, live.begin() + k_ConfigurationMapVariantOffset + k_MapVariantSize);
        std::memset(block.data() + k_MapVariantBoundsOffset, bounds, k_MapVariantBoundsSize);
        return block;
    }
}

TEST_F(MapVariantServiceTests, PrepareLiveChangesNothingAndCommitLiveGivesTheRuntimeTheBlockWithItsOwnBoundsAndTheOptionsTheBlock)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    const auto pick{ this->Pick("reach bus") };

    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };
    const Buffer runtimeBefore{ runtime };
    const Buffer optionsBefore{ options };
    const Buffer block{ this->Read("reach bus") };

    const auto plan{ m_Service.PrepareLive(pick, runtime) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(runtime, runtimeBefore);
    EXPECT_EQ(options, optionsBefore);
    EXPECT_EQ(m_Store.GetActive(), nullptr);

    ASSERT_TRUE(m_Service.CommitLive(*plan, runtime, options).has_value());

    EXPECT_EQ(options, block);
    for (std::size_t offset{ 0 }; offset < k_MapVariantSize; ++offset)
    {
        const bool isBounds{ offset >= k_MapVariantBoundsOffset && offset < k_MapVariantBoundsOffset + k_MapVariantBoundsSize };
        ASSERT_EQ(runtime[offset], isBounds ? runtimeBefore[offset] : block[offset]) << "offset 0x" << std::hex << offset;
    }
}

TEST_F(MapVariantServiceTests, PrepareLiveOfAnotherMapIsRefusedWithAReason)
{
    this->WriteConfig("other map", MakeConfig(0x11, 145, 3010, 0x5C));
    const auto pick{ this->Pick("other map") };

    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };

    const auto plan{ m_Service.PrepareLive(pick, runtime) };

    ASSERT_FALSE(plan.has_value());
    EXPECT_NE(plan.error().find("map id 3010 instead of 3006"), std::string::npos);
}

TEST_F(MapVariantServiceTests, NoneBringsBackTheMapVariantTheStartCarried)
{
    const Buffer carried{ MakeConfig(0x77, 145, 3006, 0xA1) };
    m_Service.BeginStart(carried);

    Buffer runtime{ MakeLive(0x5C, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0x5C, 145, 3006, 0x02) };

    const auto plan{ m_Service.PrepareLive(nullptr, runtime) };

    ASSERT_TRUE(plan.has_value());
    ASSERT_TRUE(m_Service.CommitLive(*plan, runtime, options).has_value());
    EXPECT_TRUE(std::equal(options.begin(), options.end(), carried.begin() + k_ConfigurationMapVariantOffset));
}

TEST_F(MapVariantServiceTests, NoneWithoutAStartIsRefused)
{
    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };

    const auto plan{ m_Service.PrepareLive(nullptr, runtime) };

    ASSERT_FALSE(plan.has_value());
    EXPECT_NE(plan.error().find("not known"), std::string::npos);
}

TEST_F(MapVariantServiceTests, CheckComparesWithTheMapVariantTheStartCarriedOnlyOnceThereIsOne)
{
    this->WriteConfig("other map", MakeConfig(0x11, 145, 3010, 0x5C));
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    const auto other{ this->Pick("other map") };
    const auto fits{ this->Pick("reach bus") };

    // Before a start nothing is compared.
    EXPECT_FALSE(m_Service.Check(*other).has_value());

    m_Service.BeginStart(MakeConfig(0x77, 145, 3006, 0xA1));

    EXPECT_TRUE(m_Service.Check(*other).has_value());
    EXPECT_FALSE(m_Service.Check(*fits).has_value());
}

TEST_F(MapVariantServiceTests, ACommitThatCannotWriteLeavesRuntimeAndOptionsAsTheyWere)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    const auto pick{ this->Pick("reach bus") };
    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    const auto plan{ m_Service.PrepareLive(pick, runtime) };
    ASSERT_TRUE(plan.has_value());

    Buffer small(k_MapVariantSize - 1, std::byte{ 0x44 });
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };
    const Buffer runtimeBefore{ runtime };
    const Buffer optionsBefore{ options };

    EXPECT_FALSE(m_Service.CommitLive(*plan, small, options).has_value());
    EXPECT_EQ(runtime, runtimeBefore);
    EXPECT_EQ(options, optionsBefore);
}


namespace
{
    // return: The map variant of MakeLive with a label table: the strings in order and the placements' labels untouched (none in use).
    auto MakeLabeledLive(std::uint8_t fill, const std::vector<std::string>& labels) -> Buffer
    {
        Buffer block{ MakeLive(fill, 145, 3006, 0x01) };
        std::size_t offset{ 0 };
        for (std::size_t index{ 0 }; index < labels.size(); ++index)
        {
            std::memcpy(block.data() + k_MapVariantLabelStringsOffset + offset, labels[index].c_str(), labels[index].size() + 1);
            const std::uint16_t at{ static_cast<std::uint16_t>(offset) };
            std::memcpy(block.data() + k_MapVariantLabelOffsetsOffset + index * 2, &at, sizeof(at));
            offset += labels[index].size() + 1;
        }

        const std::int32_t count{ static_cast<std::int32_t>(labels.size()) };
        std::memcpy(block.data() + k_MapVariantLabelCountOffset, &count, sizeof(count));
        return block;
    }

    auto CountOf(const Buffer& block) -> std::int32_t
    {
        std::int32_t count{};
        std::memcpy(&count, block.data() + k_MapVariantLabelCountOffset, sizeof(count));
        return count;
    }
}

TEST_F(MapVariantServiceTests, TheLabelsOfTheMatchKeepTheirIndexInTheRuntimeAndInTheOptions)
{
    // The map variant of the config has [flag_stand, teleporter]; the live one [teleport, flag_stand].
    Buffer config{ MakeConfig(0x11, 145, 3006, 0x5C) };
    const Buffer incomingBlock{ MakeLabeledLive(0x5C, { "flag_stand", "teleporter" }) };
    std::ranges::copy(incomingBlock, config.begin() + k_ConfigurationMapVariantOffset);
    this->WriteConfig("open flag", config);
    const auto pick{ this->Pick("open flag") };

    Buffer runtime{ MakeLabeledLive(0xA1, { "teleport", "flag_stand" }) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };

    const auto plan{ m_Service.PrepareLive(pick, runtime) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->LabelsMoved, 2u);
    EXPECT_EQ(plan->LabelsAppended, 1u);
    EXPECT_EQ(plan->LabelsKept, 1u);
    EXPECT_NE(plan->Block, incomingBlock);
    EXPECT_EQ(CountOf(plan->Block), 3);
    EXPECT_EQ(CountOf(plan->Composed), 3);

    ASSERT_TRUE(m_Service.CommitLive(*plan, runtime, options).has_value());
    // Both copies hold the same label table (strings, offsets and count), so a copy of one over the other brings no other order back.
    EXPECT_EQ(options, plan->Block);
    EXPECT_EQ(CountOf(runtime), 3);
    EXPECT_EQ(CountOf(options), 3);
    const auto tableBegin{ static_cast<std::ptrdiff_t>(k_MapVariantLabelStringsOffset) };
    const auto tableEnd{ static_cast<std::ptrdiff_t>(k_MapVariantLabelCountOffset + 4) };
    EXPECT_TRUE(std::equal(runtime.begin() + tableBegin, runtime.begin() + tableEnd, options.begin() + tableBegin));
    EXPECT_TRUE(Logged(LogLevel::Info, "labels remapped: 2 moved, 1 appended"));
}

TEST_F(MapVariantServiceTests, AMapVariantWhoseLabelsDoNotFitIsRefusedWholeWithTheReason)
{
    Buffer config{ MakeConfig(0x11, 145, 3006, 0x5C) };
    std::ranges::copy(MakeLabeledLive(0x5C, { "extra" }), config.begin() + k_ConfigurationMapVariantOffset);
    this->WriteConfig("many", config);
    const auto pick{ this->Pick("many") };

    std::vector<std::string> many{};
    for (std::size_t index{ 0 }; index < 0x100; ++index) many.push_back(std::format("l{}", index));
    const Buffer runtime{ MakeLabeledLive(0xA1, many) };

    const auto plan{ m_Service.PrepareLive(pick, runtime) };

    ASSERT_FALSE(plan.has_value());
    EXPECT_NE(plan.error().find("labels: 257 labels do not fit in 256"), std::string::npos);
}
