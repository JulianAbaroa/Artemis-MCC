#include <gtest/gtest.h>
#include <process.h>

import Common.Log.System;
import Common.Log.Type;
import Services.Settings.State;
import Engine.Base.MapVariant.Type;
import Engine.Base.MapVariant.State;
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
    using Engine::Base::MapVariant::Type::MapVariantOutcome;
    using Engine::Match::State::MatchConfigStore;
    using Engine::Match::System::MapVariantService;
    using Engine::Match::System::MatchConfigService;
    using Services::Settings::State::SettingsStore;
    using Tests::Support::Log::CollectingSink;

    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
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
    protected:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        SettingsStore m_SettingsStore{};
        MatchConfigStore m_ConfigStore{};
        MatchConfigService m_ConfigService{ m_Log, m_SettingsStore, m_ConfigStore };
        MapVariantStore m_Store{};
        MapVariantService m_Service{ m_Log, m_ConfigService, m_Store };
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
            return config;
        }

        auto WriteConfig(const std::string& name, const Buffer& bytes) const -> void
        {
            const path library{ m_Folder / "MatchConfigs" };
            std::filesystem::create_directories(library);
            std::ofstream stream{ library / (name + ".bin"), std::ios::binary | std::ios::trunc };
            stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
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

TEST_F(MapVariantServiceTests, SetActiveReadsTheMapVariantOfTheConfig)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));

    EXPECT_TRUE(m_Service.SetActive("reach bus"));

    EXPECT_TRUE(m_Service.HasActive());
    EXPECT_EQ(m_Store.GetActiveName(), "reach bus");
    ASSERT_NE(m_Store.GetActive(), nullptr);
    EXPECT_EQ(m_Store.GetActive()->Block.size(), k_MapVariantSize);
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::None);
    EXPECT_TRUE(Logged(LogLevel::Info, "map id 3006, 145 quotas"));
}

TEST_F(MapVariantServiceTests, SetActiveOfAMissingConfigFailsAndKeepsTheActiveOne)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("reach bus"));

    EXPECT_FALSE(m_Service.SetActive("missing"));

    EXPECT_EQ(m_Store.GetActiveName(), "reach bus");
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Failed);
    EXPECT_EQ(m_Store.GetResult().Name, "missing");
    EXPECT_TRUE(Logged(LogLevel::Error, "could not be used"));
}

TEST_F(MapVariantServiceTests, ClearActiveRemovesItAndTheResult)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("reach bus"));

    m_Service.ClearActive();

    EXPECT_FALSE(m_Service.HasActive());
    EXPECT_TRUE(m_Store.GetActiveName().empty());
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::None);
}

TEST_F(MapVariantServiceTests, ApplyToStartWritesTheBlockAndNothingOutsideIt)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("reach bus"));

    const Buffer original{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{ original };

    EXPECT_EQ(m_Service.ApplyToStart(start), MapVariantOutcome::AppliedAtLoad);

    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::AppliedAtLoad);
    EXPECT_EQ(m_Store.GetResult().Name, "reach bus");
    EXPECT_TRUE(DifferOnlyInMapVariant(start, original));

    const Buffer expected{ MakeConfig(0x11, 145, 3006, 0x5C) };
    EXPECT_TRUE(std::equal(start.begin() + k_ConfigurationMapVariantOffset, start.begin() + k_ConfigurationMapVariantOffset + k_MapVariantSize,
        expected.begin() + k_ConfigurationMapVariantOffset));
}

TEST_F(MapVariantServiceTests, ApplyToStartRejectsAnotherMapAndLeavesTheBufferEqual)
{
    this->WriteConfig("other map", MakeConfig(0x11, 145, 3010, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("other map"));

    const Buffer original{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{ original };

    EXPECT_EQ(m_Service.ApplyToStart(start), MapVariantOutcome::Rejected);

    EXPECT_EQ(start, original);
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Rejected);
    EXPECT_NE(m_Store.GetResult().Message.find("map id 3010 instead of 3006"), std::string::npos);
    EXPECT_TRUE(Logged(LogLevel::Warning, "does not fit the map of the start"));
}

TEST_F(MapVariantServiceTests, ApplyToStartRejectsAnotherNumberOfQuotas)
{
    this->WriteConfig("other quotas", MakeConfig(0x11, 120, 3006, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("other quotas"));

    const Buffer original{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{ original };

    EXPECT_EQ(m_Service.ApplyToStart(start), MapVariantOutcome::Rejected);
    EXPECT_EQ(start, original);
    EXPECT_NE(m_Store.GetResult().Message.find("120 quotas instead of 145"), std::string::npos);
}

TEST_F(MapVariantServiceTests, ApplyToStartWithoutAnActiveMapVariantTouchesNothing)
{
    const Buffer original{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{ original };

    EXPECT_EQ(m_Service.ApplyToStart(start), MapVariantOutcome::NoMap);

    EXPECT_EQ(start, original);
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::None);
}

TEST_F(MapVariantServiceTests, ApplyToStartRefusesABufferTooSmall)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("reach bus"));

    Buffer start(k_ConfigurationMapVariantOffset + k_MapVariantSize - 1, std::byte{ 0x77 });
    const Buffer original{ start };

    EXPECT_EQ(m_Service.ApplyToStart(start), MapVariantOutcome::Failed);
    EXPECT_EQ(start, original);
}

TEST_F(MapVariantServiceTests, TheActiveMapVariantGoesOverTheTransplantedOneAndTheRestOfTheTransplantStays)
{
    // The order of the start: transplant, then the active map variant (D266).
    this->WriteConfig("recorded", MakeConfig(0x22, 145, 3006, 0x33));
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_ConfigService.LoadPending("recorded"));
    ASSERT_TRUE(m_Service.SetActive("reach bus"));

    const Buffer mcc{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{};
    ASSERT_TRUE(m_ConfigService.Transplant(mcc.data(), start));

    const Buffer transplanted{ start };
    EXPECT_EQ(static_cast<std::uint8_t>(transplanted[k_ConfigurationMapVariantOffset + 0x10]), 0x33);

    EXPECT_EQ(m_Service.ApplyToStart(start), MapVariantOutcome::AppliedAtLoad);

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