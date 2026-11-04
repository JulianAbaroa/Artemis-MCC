#include <gtest/gtest.h>
#include <process.h>

import Common.Log.System;
import Common.Log.Type;
import Services.Settings.State;
import Engine.Base.Preset.Type;
import Engine.Base.Preset.State;
import Engine.Base.Preset.System;
import Engine.Match.Type;
import Blam.Layout.Game.Type;
import Tests.Support.Log;
import std;

// PresetStore and PresetService (Q97, D264, D265): the compiled presets are read from the storage folder and the active one is written into
// the copy of the start configuration. The service works on a temporary storage folder with the shared fixture of Blam.Layout, and the
// configuration is built by the test. Nothing here needs MCC.

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Engine::Base::Preset::State::PresetStore;
    using Engine::Base::Preset::System::PresetService;
    using Engine::Base::Preset::Type::PresetOutcome;
    using Engine::Base::Preset::Type::PresetResult;
    using Engine::Base::Preset::Type::k_ConfigurationVariantOffset;
    using Services::Settings::State::SettingsStore;
    using Tests::Support::Log::CollectingSink;
    using Blam::Layout::Game::Type::VariantPreset::CompiledPreset;
    using Blam::Layout::Game::Type::VariantPreset::PresetBase;
    using Blam::Layout::Game::Type::Offset::k_RoundStateBetween;
    using Blam::Layout::Game::Type::Offset::k_RoundStateEnded;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineType;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineTypeMegalo;
    using Blam::Layout::Game::Type::Offset::k_VariantMegaloTraitSetCount;
    using Blam::Layout::Game::Type::Offset::k_VariantRespawnTime;
    using Blam::Layout::Game::Type::Offset::k_VariantRoundLimit;
    using Engine::Match::Type::k_EngineConfigurationSize;

    using std::filesystem::path;

    // Round state of the engine while a round is running (1), and the mode of a match (3). Writes at a round break need neither.
    constexpr std::uint8_t k_RoundStateInPlay{ 1 };
    constexpr std::uint8_t k_ModeMultiplayer{ 3 };
    constexpr std::uint8_t k_ModeNone{ 0 };

    // A preset that changes only what a round break can change: respawn time and round limit.
    constexpr std::string_view k_BetweenPreset{ "preset between\nsource 0\nwrite 2c7 spawn 03 ff\nwrite 2bf round_end 03 ff\n" };

    constexpr std::string_view k_RejectedPreset{ "preset rejected\nsource 0\nwrite 2c7 spawn 03 ff\ncheck megalo_trait_set 5\n" };

    class PresetTests : public testing::Test
    {
    protected:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        SettingsStore m_SettingsStore{};
        PresetStore m_Store{};
        PresetService m_Service{ m_Log, m_SettingsStore, m_Store };
        path m_Folder{};

        auto SetUp() -> void override
        {
            const std::string test{ testing::UnitTest::GetInstance()->current_test_info()->name() };
            m_Folder = std::filesystem::temp_directory_path() / std::format("artemis_tests_preset_{}_{}", _getpid(), test);

            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
            std::filesystem::create_directories(m_Folder / "Presets", error);

            // The shared fixture of Blam.Layout: the catalog and the preset the Python compiler writes.
            // This file is Artemis/7_Tests/Tests/Engine/PresetTests.cpp.
            const path source{ std::source_location::current().file_name() };
            const path fixture{ source.parent_path().parent_path() / "Blam" / "Fixtures" / "presets" };
            for (const char* name : { "_catalog.txt", "fixture.txt" }) std::filesystem::copy_file(fixture / name, m_Folder / "Presets" / name, error);

            m_SettingsStore.SetAppDataDirectory(m_Folder.string());
        }

        auto TearDown() -> void override
        {
            std::error_code error{};
            std::filesystem::remove_all(m_Folder, error);
        }

        auto WritePreset(std::string_view name, std::string_view text) const -> void
        {
            std::ofstream stream{ m_Folder / "Presets" / (std::string{ name } + ".txt"), std::ios::binary | std::ios::trunc };
            stream << text;
        }

        // return: A start configuration of MCC with a Megalo variant of 3 trait sets and a recognizable value in every byte of the variant.
        static auto MakeConfiguration() -> std::vector<std::byte>
        {
            std::vector<std::byte> configuration(k_EngineConfigurationSize);
            for (std::size_t i{}; i < configuration.size(); ++i) configuration[i] = static_cast<std::byte>(0x40 + i % 7);

            const std::uint32_t megalo{ k_VariantEngineTypeMegalo };
            const std::uint32_t sets{ 3 };
            std::memcpy(configuration.data() + k_ConfigurationVariantOffset + k_VariantEngineType, &megalo, sizeof(megalo));
            std::memcpy(configuration.data() + k_ConfigurationVariantOffset + k_VariantMegaloTraitSetCount, &sets, sizeof(sets));
            return configuration;
        }

        // return: The live variant of a match: the variant of MakeConfiguration alone.
        static auto MakeVariant() -> std::vector<std::uint8_t>
        {
            const std::vector<std::byte> configuration{ MakeConfiguration() };
            std::vector<std::uint8_t> variant(configuration.size() - k_ConfigurationVariantOffset);
            std::memcpy(variant.data(), configuration.data() + k_ConfigurationVariantOffset, variant.size());
            return variant;
        }

        // What the start hook does for every start: the per-start state first, then the preset only if one is active.
        auto Start(std::span<std::byte> configuration) -> PresetOutcome
        {
            m_Service.BeginStart();
            return m_Service.HasActive() ? m_Service.ApplyToStart(configuration) : PresetOutcome::NoPreset;
        }

        // return: The live variant of a start configuration.
        static auto VariantOf(const std::vector<std::byte>& configuration) -> std::vector<std::uint8_t>
        {
            std::vector<std::uint8_t> variant(configuration.size() - k_ConfigurationVariantOffset);
            std::memcpy(variant.data(), configuration.data() + k_ConfigurationVariantOffset, variant.size());
            return variant;
        }

        static auto VariantByte(const std::vector<std::byte>& configuration, std::size_t offset) -> int
        {
            return std::to_integer<int>(configuration[k_ConfigurationVariantOffset + offset]);
        }

        auto Logged(LogLevel level, std::string_view text) const -> bool
        {
            return std::ranges::any_of(m_Sink.m_Messages, [&](const LogMessage& message) {
                return message.Level == level && message.Message.find(text) != std::string::npos;
                });
        }
    };
}

TEST(PresetStoreTest, StartsWithNothingAndKeepsWhatIsSet)
{
    PresetStore store{};
    EXPECT_EQ(store.GetActive(), nullptr);
    EXPECT_EQ(store.GetCatalog(), nullptr);
    EXPECT_EQ(store.GetBase(), nullptr);
    EXPECT_TRUE(store.GetActiveName().empty());
    EXPECT_EQ(store.GetResult().Outcome, PresetOutcome::None);

    auto preset{ std::make_shared<CompiledPreset>() };
    preset->Name = "demo";
    store.SetActive(preset);
    store.SetBase(std::make_shared<PresetBase>());
    store.SetResult(PresetResult{ .Outcome = PresetOutcome::Rejected, .Name = "demo", .Message = "why" });

    EXPECT_EQ(store.GetActiveName(), "demo");
    EXPECT_NE(store.GetBase(), nullptr);
    EXPECT_EQ(store.GetResult().Outcome, PresetOutcome::Rejected);
    EXPECT_EQ(store.GetResult().Message, "why");

    // A reader keeps its snapshot when the preset is cleared.
    const auto snapshot{ store.GetActive() };
    store.SetActive(nullptr);
    EXPECT_EQ(store.GetActive(), nullptr);
    ASSERT_NE(snapshot, nullptr);
    EXPECT_EQ(snapshot->Name, "demo");
}

TEST_F(PresetTests, SetActiveReadsTheCompiledFilesFromTheStorageFolder)
{
    EXPECT_FALSE(m_Service.HasActive());
    ASSERT_TRUE(m_Service.SetActive("fixture"));

    EXPECT_TRUE(m_Service.HasActive());
    EXPECT_EQ(m_Store.GetActiveName(), "fixture");
    ASSERT_NE(m_Store.GetCatalog(), nullptr);
    EXPECT_GT(m_Store.GetCatalog()->Fields.size(), 900u);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::None);

    m_Service.ClearActive();
    EXPECT_FALSE(m_Service.HasActive());
}

TEST_F(PresetTests, SetActiveFailsWithAnErrorAndKeepsTheActivePreset)
{
    ASSERT_TRUE(m_Service.SetActive("fixture"));

    for (const std::string_view bad : { "missing", "", "Fixture", "../fixture", "has space", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" })
    {
        EXPECT_FALSE(m_Service.SetActive(bad)) << bad;
        EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Failed) << bad;
        EXPECT_EQ(m_Store.GetActiveName(), "fixture") << bad;
    }
    EXPECT_TRUE(Logged(LogLevel::Error, "\"missing\" not set"));

    WritePreset("broken", "preset broken\nwrite zz load 00 ff\n");
    EXPECT_FALSE(m_Service.SetActive("broken"));
    EXPECT_NE(m_Store.GetResult().Message.find("line 2"), std::string::npos);

    WritePreset("other", "preset another\nsource 0\n");
    EXPECT_FALSE(m_Service.SetActive("other"));

    m_SettingsStore.SetAppDataDirectory("");
    EXPECT_FALSE(m_Service.SetActive("fixture"));
    EXPECT_EQ(m_Store.GetActiveName(), "fixture");
}

TEST_F(PresetTests, ApplyToStartWritesThePresetIntoTheVariantOfTheCopyAndKeepsTheBase)
{
    ASSERT_TRUE(m_Service.SetActive("fixture"));
    const std::vector<std::byte> original{ MakeConfiguration() };
    std::vector<std::byte> start{ original };

    EXPECT_EQ(m_Service.ApplyToStart(start), PresetOutcome::AppliedAtLoad);

    EXPECT_EQ(VariantByte(start, k_VariantRespawnTime), 3);
    EXPECT_EQ(VariantByte(start, k_VariantRoundLimit), 3);
    // Friendly fire is bit 0 of the social flags and the powerups bit of the map flags is cleared, the other bits stay.
    EXPECT_EQ(VariantByte(start, 0x300), VariantByte(original, 0x300) & ~0x08);
    EXPECT_EQ(VariantByte(start, 0x2FC), VariantByte(original, 0x2FC) | 0x01);

    // Only bytes of the variant changed, never the ones before it.
    for (std::size_t i{}; i < k_ConfigurationVariantOffset; ++i) ASSERT_EQ(start[i], original[i]) << i;
    EXPECT_NE(start, original);

    const PresetResult result{ m_Store.GetResult() };
    EXPECT_EQ(result.Outcome, PresetOutcome::AppliedAtLoad);
    EXPECT_EQ(result.Name, "fixture");
    EXPECT_TRUE(result.Message.empty());

    // The base is what the variant held before: respawn time and round limit are the original bytes.
    const auto base{ m_Store.GetBase() };
    ASSERT_NE(base, nullptr);
    EXPECT_FALSE(base->Bytes.empty());
    EXPECT_NE(std::ranges::find(base->Bytes, std::to_integer<std::uint8_t>(original[k_ConfigurationVariantOffset + k_VariantRespawnTime])), base->Bytes.end());
}

TEST_F(PresetTests, ApplyToStartWithoutAnActivePresetChangesNothing)
{
    const std::vector<std::byte> original{ MakeConfiguration() };
    std::vector<std::byte> start{ original };

    EXPECT_EQ(m_Service.ApplyToStart(start), PresetOutcome::NoPreset);
    EXPECT_EQ(start, original);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::None);
    EXPECT_EQ(m_Store.GetBase(), nullptr);

    ASSERT_TRUE(m_Service.SetActive("fixture"));
    m_Service.ClearActive();
    EXPECT_EQ(m_Service.ApplyToStart(start), PresetOutcome::NoPreset);
    EXPECT_EQ(start, original);
}

TEST_F(PresetTests, AVariantThatIsNotMegaloGetsNoPreset)
{
    ASSERT_TRUE(m_Service.SetActive("fixture"));
    std::vector<std::byte> start{ MakeConfiguration() };
    const std::uint32_t other{ 1 };
    std::memcpy(start.data() + k_ConfigurationVariantOffset + k_VariantEngineType, &other, sizeof(other));
    const std::vector<std::byte> before{ start };

    EXPECT_EQ(m_Service.ApplyToStart(start), PresetOutcome::NoPreset);
    EXPECT_EQ(start, before);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::NoPreset);
    EXPECT_TRUE(Logged(LogLevel::Warning, "not a Megalo variant"));
}

TEST_F(PresetTests, ATraitSetTheScriptDoesNotHaveRejectsThePresetAndLeavesTheCopyAsItWas)
{
    WritePreset("rejected", k_RejectedPreset);
    ASSERT_TRUE(m_Service.SetActive("rejected"));
    std::vector<std::byte> start{ MakeConfiguration() };
    const std::vector<std::byte> before{ start };

    EXPECT_EQ(m_Service.ApplyToStart(start), PresetOutcome::Rejected);
    EXPECT_EQ(start, before);
    EXPECT_NE(m_Store.GetResult().Message.find("trait set 5"), std::string::npos);
    EXPECT_TRUE(Logged(LogLevel::Warning, "trait set 5"));
    EXPECT_EQ(m_Store.GetBase(), nullptr);
}

TEST_F(PresetTests, ACopyTooSmallToHoldAVariantFailsWithAnError)
{
    ASSERT_TRUE(m_Service.SetActive("fixture"));
    std::vector<std::byte> start(k_ConfigurationVariantOffset, std::byte{ 0x11 });

    EXPECT_EQ(m_Service.ApplyToStart(start), PresetOutcome::Failed);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Failed);
    EXPECT_TRUE(Logged(LogLevel::Error, "too small"));
    EXPECT_EQ(start, std::vector<std::byte>(k_ConfigurationVariantOffset, std::byte{ 0x11 }));
}

TEST_F(PresetTests, ARequestInAMatchWaitsForAnEndedOrBetweenRoundsAndNeverWritesInPlay)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(m_Service.Request("between", true));
    EXPECT_TRUE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Pending);
    EXPECT_EQ(m_Store.GetResult().Name, "between");

    m_Service.ApplyBetweenRounds(variant, k_RoundStateInPlay, k_ModeMultiplayer);
    m_Service.ApplyBetweenRounds(variant, 0, k_ModeMultiplayer);
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeNone);
    EXPECT_EQ(variant, original);
    EXPECT_TRUE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetBase(), nullptr);

    m_Service.ApplyBetweenRounds(variant, k_RoundStateEnded, k_ModeMultiplayer);
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_NE(variant, original);
    EXPECT_EQ(variant[k_VariantRespawnTime], 3);
    EXPECT_EQ(variant[k_VariantRoundLimit], 3);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Applied);
    EXPECT_EQ(m_Store.GetActiveName(), "between");
}

TEST_F(PresetTests, TheBaseIsSavedAtTheFirstRequestOfAMatchLoadedWithoutAPresetAndKeptUntilTheNextLoad)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };
    EXPECT_EQ(m_Store.GetBase(), nullptr);

    ASSERT_TRUE(m_Service.Request("between", true));
    // The base is not taken at the request but at the break, from the variant as it is then.
    EXPECT_EQ(m_Store.GetBase(), nullptr);
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);

    const auto base{ m_Store.GetBase() };
    ASSERT_NE(base, nullptr);
    EXPECT_NE(std::ranges::find(base->Bytes, original[k_VariantRespawnTime]), base->Bytes.end());

    // A second request keeps the same base and the preset after it is applied again over the first one.
    ASSERT_TRUE(m_Service.Request("between", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    EXPECT_EQ(m_Store.GetBase(), base);

    // The next start drops it, with or without a preset.
    std::vector<std::byte> start{ MakeConfiguration() };
    EXPECT_EQ(this->Start(start), PresetOutcome::AppliedAtLoad);
    EXPECT_NE(m_Store.GetBase(), base);

    m_Service.ClearActive();
    EXPECT_EQ(this->Start(start), PresetOutcome::NoPreset);
    EXPECT_EQ(m_Store.GetBase(), nullptr);
}

TEST_F(PresetTests, NoPresetBetweenRoundsRestoresTheBaseAndClearsTheActivePreset)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(m_Service.Request("between", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    ASSERT_NE(variant, original);

    ASSERT_TRUE(m_Service.Request("", true));
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Pending);
    m_Service.ApplyBetweenRounds(variant, k_RoundStateInPlay, k_ModeMultiplayer);
    EXPECT_NE(variant, original);

    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    EXPECT_EQ(variant, original);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Applied);
    EXPECT_TRUE(m_Store.GetResult().Name.empty());
}

TEST_F(PresetTests, NoPresetWithNothingWrittenLeavesTheVariantAlone)
{
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(m_Service.Request("", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);

    EXPECT_EQ(variant, original);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Applied);
    EXPECT_FALSE(m_Service.HasPending());
}

TEST_F(PresetTests, AFieldOnlyALoadCanChangeRejectsTheRequestAndKeepsTheVariantAndTheActivePreset)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };

    ASSERT_TRUE(m_Service.Request("between", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    const std::vector<std::uint8_t> applied{ variant };

    // The fixture changes load fields the match was not loaded with.
    ASSERT_TRUE(m_Service.Request("fixture", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);

    EXPECT_EQ(variant, applied);
    EXPECT_EQ(m_Store.GetActiveName(), "between");
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Rejected);
    EXPECT_EQ(m_Store.GetResult().Name, "fixture");
    EXPECT_NE(m_Store.GetResult().Message.find("only change at load"), std::string::npos);
    EXPECT_FALSE(m_Service.HasPending());
}

TEST_F(PresetTests, ATraitSetTheScriptLacksRejectsTheRequestBetweenRounds)
{
    WritePreset("rejected", k_RejectedPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(m_Service.Request("rejected", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateEnded, k_ModeMultiplayer);

    EXPECT_EQ(variant, original);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Rejected);
    EXPECT_NE(m_Store.GetResult().Message.find("trait set 5"), std::string::npos);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
}

TEST_F(PresetTests, AVariantThatIsNotMegaloRejectsTheRequestBetweenRounds)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    variant[k_VariantEngineType] = 1;
    const std::vector<std::uint8_t> before{ variant };

    ASSERT_TRUE(m_Service.Request("between", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);

    EXPECT_EQ(variant, before);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Rejected);
}

TEST_F(PresetTests, ARequestInTheLobbyBecomesTheActivePresetForTheNextStart)
{
    ASSERT_TRUE(m_Service.Request("fixture", false));

    EXPECT_EQ(m_Store.GetActiveName(), "fixture");
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::AppliedAtLoad);

    std::vector<std::byte> start{ MakeConfiguration() };
    EXPECT_EQ(m_Service.ApplyToStart(start), PresetOutcome::AppliedAtLoad);

    // None in the lobby just clears it.
    ASSERT_TRUE(m_Service.Request("", false));
    EXPECT_EQ(m_Store.GetActive(), nullptr);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::AppliedAtLoad);
}

TEST_F(PresetTests, ARequestWhoseFilesAreMissingFailsAtOnceAndKeepsTheActivePreset)
{
    ASSERT_TRUE(m_Service.SetActive("fixture"));

    EXPECT_FALSE(m_Service.Request("missing", true));
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Failed);
    EXPECT_EQ(m_Store.GetActiveName(), "fixture");

    EXPECT_FALSE(m_Service.Request("missing", false));
    EXPECT_EQ(m_Store.GetActiveName(), "fixture");
}

TEST_F(PresetTests, ARequestNoRoundBreakServedIsWrittenByTheNextStart)
{
    ASSERT_TRUE(m_Service.Request("fixture", true));
    std::vector<std::byte> start{ MakeConfiguration() };

    EXPECT_EQ(this->Start(start), PresetOutcome::AppliedAtLoad);
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetActiveName(), "fixture");
}

TEST_F(PresetTests, ARequestForNoneNoRoundBreakServedClearsTheActivePresetAtTheNextStart)
{
    ASSERT_TRUE(m_Service.SetActive("fixture"));
    ASSERT_TRUE(m_Service.Request("", true));
    std::vector<std::byte> start{ MakeConfiguration() };
    const std::vector<std::byte> original{ start };

    EXPECT_EQ(this->Start(start), PresetOutcome::NoPreset);
    EXPECT_EQ(start, original);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::AppliedAtLoad);
}

TEST_F(PresetTests, EachMatchComposesOverItsOwnBaseThroughThePathOfTheStartHook)
{
    WritePreset("a", "preset a\nsource 0\nwrite 2c7 spawn 03 ff\nwrite 2bf round_end 03 ff\n");
    WritePreset("b", "preset b\nsource 0\nwrite 2bf round_end 04 ff\n");

    // Match 1 has no preset, gets A, then none.
    std::vector<std::byte> first{ MakeConfiguration() };
    ASSERT_EQ(this->Start(first), PresetOutcome::NoPreset);
    std::vector<std::uint8_t> live1{ VariantOf(first) };
    const std::vector<std::uint8_t> original1{ live1 };

    ASSERT_TRUE(m_Service.Request("a", true));
    m_Service.ApplyBetweenRounds(live1, k_RoundStateBetween, k_ModeMultiplayer);
    ASSERT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Applied);
    EXPECT_EQ(live1[k_VariantRespawnTime], 3);
    const auto base1{ m_Store.GetBase() };
    ASSERT_NE(base1, nullptr);

    ASSERT_TRUE(m_Service.Request("", true));
    m_Service.ApplyBetweenRounds(live1, k_RoundStateBetween, k_ModeMultiplayer);
    EXPECT_EQ(live1, original1);

    // Match 2 starts with another respawn time and gets B at load: B does not write it, so it stays the one of match 2.
    ASSERT_TRUE(m_Service.Request("b", false));
    std::vector<std::byte> second{ MakeConfiguration() };
    second[k_ConfigurationVariantOffset + k_VariantRespawnTime] = std::byte{ 0x55 };
    ASSERT_EQ(this->Start(second), PresetOutcome::AppliedAtLoad);
    EXPECT_EQ(VariantByte(second, k_VariantRespawnTime), 0x55);
    EXPECT_EQ(VariantByte(second, k_VariantRoundLimit), 4);

    const auto base2{ m_Store.GetBase() };
    ASSERT_NE(base2, nullptr);
    EXPECT_NE(base2, base1);
    EXPECT_NE(std::ranges::find(base2->Bytes, std::uint8_t{ 0x55 }), base2->Bytes.end());

    // Back to none in match 2 gives its own values, not the ones of match 1.
    std::vector<std::uint8_t> live2{ VariantOf(second) };
    ASSERT_TRUE(m_Service.Request("", true));
    m_Service.ApplyBetweenRounds(live2, k_RoundStateBetween, k_ModeMultiplayer);
    EXPECT_EQ(live2[k_VariantRespawnTime], 0x55);
    EXPECT_EQ(live2[k_VariantRoundLimit], VariantByte(MakeConfiguration(), k_VariantRoundLimit));
}

TEST_F(PresetTests, TheChoiceOfTheBenchDropsARequestOfTheBridgeThatStillWaits)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(m_Service.Request("between", true));
    ASSERT_TRUE(m_Service.SetActive("fixture"));
    EXPECT_FALSE(m_Service.HasPending());
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    EXPECT_EQ(variant, original);
    EXPECT_EQ(m_Store.GetActiveName(), "fixture");

    ASSERT_TRUE(m_Service.Request("between", true));
    m_Service.ClearActive();
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetActive(), nullptr);
}

TEST_F(PresetTests, ARequestWhoseFilesFailLeavesTheEarlierWaitingRequestAlone)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };

    ASSERT_TRUE(m_Service.Request("between", true));
    EXPECT_FALSE(m_Service.Request("missing", true));
    EXPECT_TRUE(m_Service.HasPending());

    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    EXPECT_EQ(m_Store.GetActiveName(), "between");
    EXPECT_EQ(variant[k_VariantRespawnTime], 3);
}

TEST_F(PresetTests, CancelPendingDropsTheRequestAndItsResult)
{
    WritePreset("between", k_BetweenPreset);
    ASSERT_TRUE(m_Service.Request("between", true));
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Pending);

    m_Service.CancelPending();
    EXPECT_FALSE(m_Service.HasPending());
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::None);
}

TEST_F(PresetTests, GoingFromAToBGivesBackTheBaseOfAFieldBLeavesOut)
{
    WritePreset("a", "preset a\nsource 0\nwrite 2c7 spawn 03 ff\nwrite 2bf round_end 03 ff\n");
    WritePreset("b", "preset b\nsource 0\nwrite 2bf round_end 04 ff\n");
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(m_Service.Request("a", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    ASSERT_EQ(variant[k_VariantRespawnTime], 3);

    ASSERT_TRUE(m_Service.Request("b", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Applied);
    EXPECT_EQ(variant[k_VariantRespawnTime], original[k_VariantRespawnTime]);
    EXPECT_EQ(variant[k_VariantRoundLimit], 4);
}

TEST_F(PresetTests, AFieldOnlyALoadCanChangeIsAcceptedBetweenRoundsWhenItAlreadyHasTheValue)
{
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::uint8_t current{ static_cast<std::uint8_t>(variant[0x2C4] & 0x08) };
    WritePreset("loadeq", std::format("preset loadeq\nsource 0\nwrite 2c4 load {:02x} 08\nwrite 2bf round_end 03 ff\n", current));

    ASSERT_TRUE(m_Service.Request("loadeq", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);

    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Applied) << m_Store.GetResult().Message;
    EXPECT_EQ(variant[k_VariantRoundLimit], 3);
}

TEST_F(PresetTests, AWriteThatDoesNotReadBackEqualRestoresTheVariantAndKeepsTheActivePreset)
{
    // Two fields on the same byte with overlapping masks: the second write changes the bits the first one verifies.
    {
        std::ofstream catalog{ m_Folder / "Presets" / "_catalog.txt", std::ios::binary | std::ios::trunc };
        catalog << "field 10 1 ff tick\nfield 10 1 0f tick\n";
    }
    WritePreset("overlap", "preset overlap\nsource 0\nwrite 10 tick 12 ff\nwrite 10 tick 05 0f\n");
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(m_Service.Request("overlap", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);

    EXPECT_EQ(variant, original);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Failed);
    EXPECT_NE(m_Store.GetResult().Message.find("read back"), std::string::npos);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
}

TEST_F(PresetTests, ARequestWithAnotherCatalogThanTheBaseIsRejectedWithAReason)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };

    ASSERT_TRUE(m_Service.Request("between", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);
    ASSERT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Applied);
    const std::vector<std::uint8_t> applied{ variant };

    {
        std::ofstream catalog{ m_Folder / "Presets" / "_catalog.txt", std::ios::binary | std::ios::app };
        catalog << "\nfield 20 1 ff tick\n";
    }
    ASSERT_TRUE(m_Service.Request("between", true));
    m_Service.ApplyBetweenRounds(variant, k_RoundStateBetween, k_ModeMultiplayer);

    EXPECT_EQ(variant, applied);
    EXPECT_EQ(m_Store.GetResult().Outcome, PresetOutcome::Rejected);
    EXPECT_NE(m_Store.GetResult().Message.find("catalog"), std::string::npos);
    EXPECT_EQ(m_Store.GetActiveName(), "between");
}

TEST(PresetStoreTest, TakePendingReadsAndClearsInOneStepAndAnOldRequestNeverHidesANewerResult)
{
    PresetStore store{};
    EXPECT_FALSE(store.HasPending());

    const std::uint64_t first{ store.BeginRequest() };
    store.SetPending(PresetStore::PendingRequest{ .Id = first, .Name = "a" });
    EXPECT_TRUE(store.HasPending());

    const auto taken{ store.TakePending() };
    ASSERT_TRUE(taken.has_value());
    EXPECT_EQ(taken->Name, "a");
    EXPECT_FALSE(store.HasPending());
    EXPECT_FALSE(store.TakePending().has_value());

    const std::uint64_t second{ store.BeginRequest() };
    EXPECT_GT(second, first);
    EXPECT_TRUE(store.SetResultOf(second, PresetResult{ .Outcome = PresetOutcome::Pending, .Name = "b" }));
    EXPECT_FALSE(store.SetResultOf(first, PresetResult{ .Outcome = PresetOutcome::Applied, .Name = "a" }));
    EXPECT_EQ(store.GetResult().Name, "b");
    EXPECT_EQ(store.GetResult().Outcome, PresetOutcome::Pending);
}
