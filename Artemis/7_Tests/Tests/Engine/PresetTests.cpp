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
    using Blam::Layout::Game::Type::Offset::k_VariantEngineType;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineTypeMegalo;
    using Blam::Layout::Game::Type::Offset::k_VariantMegaloTraitSetCount;
    using Blam::Layout::Game::Type::Offset::k_VariantRespawnTime;
    using Blam::Layout::Game::Type::Offset::k_VariantRoundLimit;
    using Engine::Match::Type::k_EngineConfigurationSize;

    using std::filesystem::path;

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
    EXPECT_NE(m_Store.GetResult().Message.find("Line 2"), std::string::npos);

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
    EXPECT_NE(m_Store.GetResult().Message.find("Trait set 5"), std::string::npos);
    EXPECT_TRUE(Logged(LogLevel::Warning, "Trait set 5"));
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