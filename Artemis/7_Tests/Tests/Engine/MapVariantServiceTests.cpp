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
import Blam.Layout.Game.Type;
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
    using Engine::Base::MapVariant::Type::MapVariantResult;
    using Engine::Match::State::MatchConfigStore;
    using Engine::Base::MapVariant::System::MapVariantService;
    using Engine::Match::System::MatchConfigService;
    using Services::Settings::State::SettingsStore;
    using Tests::Support::Log::CollectingSink;

    using Blam::Layout::Game::Type::Offset::k_RoundStateBetween;
    using Blam::Layout::Game::Type::Offset::k_RoundStateEnded;
    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_GameModeMultiplayer;
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsSize;
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

    EXPECT_TRUE(m_Service.SetActive("reach bus", this->Read("reach bus")));

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
    ASSERT_TRUE(m_Service.SetActive("reach bus", this->Read("reach bus")));

    EXPECT_FALSE(m_Service.SetActive("missing", this->Read("missing")));

    EXPECT_EQ(m_Store.GetActiveName(), "reach bus");
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Failed);
    EXPECT_EQ(m_Store.GetResult().Name, "missing");
    EXPECT_TRUE(Logged(LogLevel::Error, "could not be used"));
}

TEST_F(MapVariantServiceTests, ClearActiveRemovesItAndTheResult)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("reach bus", this->Read("reach bus")));

    m_Service.ClearActive();

    EXPECT_FALSE(m_Service.HasActive());
    EXPECT_TRUE(m_Store.GetActiveName().empty());
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::None);
}

TEST_F(MapVariantServiceTests, ApplyToStartWritesTheBlockAndNothingOutsideIt)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("reach bus", this->Read("reach bus")));

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
    ASSERT_TRUE(m_Service.SetActive("other map", this->Read("other map")));

    const Buffer original{ MakeConfig(0x77, 145, 3006, 0xA1) };
    Buffer start{ original };

    EXPECT_EQ(m_Service.ApplyToStart(start), MapVariantOutcome::Rejected);

    EXPECT_EQ(start, original);
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Rejected);
    EXPECT_NE(m_Store.GetResult().Message.find("map id 3010 instead of 3006"), std::string::npos);
    EXPECT_TRUE(Logged(LogLevel::Warning, "does not fit the map"));
}

TEST_F(MapVariantServiceTests, ApplyToStartRejectsAnotherNumberOfQuotas)
{
    this->WriteConfig("other quotas", MakeConfig(0x11, 120, 3006, 0x5C));
    ASSERT_TRUE(m_Service.SetActive("other quotas", this->Read("other quotas")));

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
    ASSERT_TRUE(m_Service.SetActive("reach bus", this->Read("reach bus")));

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
    ASSERT_TRUE(m_Service.SetActive("reach bus", this->Read("reach bus")));

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

// --- Request, start and between rounds ---------------------------------------------------------------------------------------------

namespace
{
    constexpr std::uint8_t k_RoundStateInPlay{ 1 };
    constexpr std::uint8_t k_LobbyMode{ 1 };

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

TEST_F(MapVariantServiceTests, ARequestInTheLobbyBecomesTheActiveOneAtTheNextLoad)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));

    EXPECT_TRUE(m_Service.Request("reach bus", this->Read("reach bus"), false));

    EXPECT_EQ(m_Store.GetActiveName(), "reach bus");
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::AppliedAtLoad);
}

TEST_F(MapVariantServiceTests, ARequestInTheMatchWaitsAndAnUnreadableOneDropsTheWaitingOne)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.Request("reach bus", this->Read("reach bus"), true));

    EXPECT_TRUE(m_Service.HasPending());
    EXPECT_FALSE(m_Service.HasActive());
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Pending);

    // The older request must not change the map later without anyone being told.
    EXPECT_FALSE(m_Service.Request("missing", this->Read("missing"), true));

    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Failed);
}

TEST_F(MapVariantServiceTests, TheBenchChoiceSupersedesARequestAndNothingOfItIsWrittenAfterwards)
{
    this->WriteConfig("a", MakeConfig(0x11, 145, 3006, 0x5C));
    this->WriteConfig("b", MakeConfig(0x12, 145, 3006, 0x5D));
    ASSERT_TRUE(m_Service.Request("a", this->Read("a"), true));
    // The game thread took the request and is applying it.
    const auto taken{ m_Store.TakePending() };
    ASSERT_TRUE(taken.has_value());
    const std::uint64_t id{ taken->Id };

    ASSERT_TRUE(m_Service.SetActive("b", this->Read("b")));

    // An application of the old request already taken by the game thread finds its id outdated.
    EXPECT_FALSE(m_Store.IsCurrent(id));
    EXPECT_FALSE(m_Store.SetResultOf(id, MapVariantResult{ .Outcome = MapVariantOutcome::Applied, .Name = "a" }));

    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };
    const Buffer runtimeBefore{ runtime };
    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateEnded, k_GameModeMultiplayer);

    EXPECT_EQ(runtime, runtimeBefore);
    EXPECT_EQ(m_Store.GetActiveName(), "b");
    EXPECT_NE(m_Store.GetResult().Outcome, MapVariantOutcome::Applied);
}

TEST_F(MapVariantServiceTests, TheRequestWaitsWhileTheRoundIsInPlayOrTheModeHasNoMapVariant)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.Request("reach bus", this->Read("reach bus"), true));

    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };
    const Buffer runtimeBefore{ runtime };
    const Buffer optionsBefore{ options };

    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateInPlay, k_GameModeMultiplayer);
    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateEnded, k_LobbyMode);

    EXPECT_TRUE(m_Service.HasPending());
    EXPECT_EQ(runtime, runtimeBefore);
    EXPECT_EQ(options, optionsBefore);
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Pending);
}

TEST_F(MapVariantServiceTests, AtTheBreakTheRuntimeGetsTheBlockWithItsOwnBoundsAndTheOptionsGetTheBlock)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.Request("reach bus", this->Read("reach bus"), true));

    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };
    const Buffer runtimeBefore{ runtime };
    const Buffer block{ this->Read("reach bus") };

    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateEnded, k_GameModeMultiplayer);

    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Applied);
    EXPECT_EQ(m_Store.GetActiveName(), "reach bus");
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(options, block);

    for (std::size_t offset{ 0 }; offset < k_MapVariantSize; ++offset)
    {
        const bool isBounds{ offset >= k_MapVariantBoundsOffset && offset < k_MapVariantBoundsOffset + k_MapVariantBoundsSize };
        ASSERT_EQ(runtime[offset], isBounds ? runtimeBefore[offset] : block[offset]) << "offset 0x" << std::hex << offset;
    }
}

TEST_F(MapVariantServiceTests, ARequestIsAppliedOnceAndTheBetweenStateServesIt)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.Request("reach bus", this->Read("reach bus"), true));

    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };

    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateBetween, k_GameModeMultiplayer);
    ASSERT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Applied);

    // Written again by the engine, a second call has nothing to serve.
    std::ranges::fill(options, std::byte{ 0x44 });
    const Buffer after{ options };
    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateEnded, k_GameModeMultiplayer);

    EXPECT_EQ(options, after);
}

TEST_F(MapVariantServiceTests, ARejectedRequestLeavesRuntimeAndOptionsByteForByte)
{
    this->WriteConfig("other map", MakeConfig(0x11, 145, 3010, 0x5C));
    ASSERT_TRUE(m_Service.Request("other map", this->Read("other map"), true));

    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };
    const Buffer runtimeBefore{ runtime };
    const Buffer optionsBefore{ options };

    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateEnded, k_GameModeMultiplayer);

    EXPECT_EQ(runtime, runtimeBefore);
    EXPECT_EQ(options, optionsBefore);
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Rejected);
    EXPECT_NE(m_Store.GetResult().Message.find("map id 3010 instead of 3006"), std::string::npos);
    EXPECT_FALSE(m_Service.HasActive());
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_TRUE(Logged(LogLevel::Warning, "does not fit the map"));
}

TEST_F(MapVariantServiceTests, NoneBringsBackTheMapVariantTheStartCarried)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    const Buffer carried{ MakeConfig(0x77, 145, 3006, 0xA1) };
    m_Service.BeginStart(carried);

    Buffer runtime{ MakeLive(0x5C, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0x5C, 145, 3006, 0x02) };
    ASSERT_TRUE(m_Service.Request("", {}, true));
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Pending);

    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateEnded, k_GameModeMultiplayer);

    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Applied);
    EXPECT_FALSE(m_Service.HasActive());
    EXPECT_TRUE(std::equal(options.begin(), options.end(), carried.begin() + k_ConfigurationMapVariantOffset));
}

TEST_F(MapVariantServiceTests, NoneWithoutAStartIsRejected)
{
    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };
    const Buffer runtimeBefore{ runtime };
    ASSERT_TRUE(m_Service.Request("", {}, true));

    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateEnded, k_GameModeMultiplayer);

    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::Rejected);
    EXPECT_EQ(runtime, runtimeBefore);
}

TEST_F(MapVariantServiceTests, ARequestNoRoundBreakServedIsTakenByTheNextStart)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.Request("reach bus", this->Read("reach bus"), true));

    const Buffer next{ MakeConfig(0x77, 145, 3006, 0xA1) };
    m_Service.BeginStart(next);

    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetActiveName(), "reach bus");

    Buffer start{ next };
    EXPECT_EQ(m_Service.ApplyToStart(start), MapVariantOutcome::AppliedAtLoad);
    EXPECT_TRUE(DifferOnlyInMapVariant(start, next));
}

TEST_F(MapVariantServiceTests, CancelPendingDropsTheRequestAndItsResult)
{
    this->WriteConfig("reach bus", MakeConfig(0x11, 145, 3006, 0x5C));
    ASSERT_TRUE(m_Service.Request("reach bus", this->Read("reach bus"), true));

    m_Service.CancelPending();

    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetResult().Outcome, MapVariantOutcome::None);
}

TEST_F(MapVariantServiceTests, TheResultOfAnOldRequestNeverHidesANewerOne)
{
    this->WriteConfig("a", MakeConfig(0x11, 145, 3006, 0x5C));
    this->WriteConfig("b", MakeConfig(0x12, 145, 3006, 0x5D));
    ASSERT_TRUE(m_Service.Request("a", this->Read("a"), true));
    const auto first{ m_Store.GetResult() };
    ASSERT_TRUE(m_Service.Request("b", this->Read("b"), true));

    EXPECT_EQ(first.Name, "a");
    EXPECT_EQ(m_Store.GetResult().Name, "b");

    Buffer runtime{ MakeLive(0xA1, 145, 3006, 0x01) };
    Buffer options{ MakeLive(0xA2, 145, 3006, 0x02) };
    m_Service.ApplyBetweenRounds(runtime, options, k_RoundStateEnded, k_GameModeMultiplayer);

    EXPECT_EQ(m_Store.GetActiveName(), "b");
}
