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
    using Engine::Base::Preset::Type::PresetPick;
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

        // What the setup service does for a preset at the start: the base of the match that ended goes, then the pick is written and
        // adopted. A pick with no preset only clears the active one.
        // return: What WriteToStart returned.
        auto Start(std::span<std::byte> configuration, const PresetPick& pick) -> std::expected<bool, std::string>
        {
            m_Service.DropBase();
            auto written{ m_Service.WriteToStart(configuration, pick) };
            if (written) m_Service.Adopt(pick);
            return written;
        }

        // What the setup service does for a preset at a round break: plan, write, adopt.
        // return: Nothing, or why the preset was not written (the variant is then as it was).
        auto Live(const PresetPick& pick, std::span<std::uint8_t> variant) -> std::expected<void, std::string>
        {
            auto plan{ m_Service.PrepareLive(pick, variant) };
            if (!plan) return std::unexpected(plan.error());

            auto committed{ m_Service.CommitLive(*plan, variant) };
            if (!committed) return committed;

            m_Service.AdoptLive(*plan);
            return {};
        }

        // return: The pick of a preset the test wrote or the fixture has.
        auto Pick(std::string_view name) const -> PresetPick
        {
            auto pick{ m_Service.Resolve(name) };
            EXPECT_TRUE(pick.has_value()) << name;
            return pick.value_or(PresetPick{});
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
    EXPECT_EQ(store.GetBaseCatalog(), nullptr);
    EXPECT_TRUE(store.GetActiveName().empty());

    auto preset{ std::make_shared<CompiledPreset>() };
    preset->Name = "demo";
    store.SetActive(preset);
    store.SetBase(std::make_shared<PresetBase>());

    EXPECT_EQ(store.GetActiveName(), "demo");
    EXPECT_NE(store.GetBase(), nullptr);

    // A reader keeps its snapshot when the preset is cleared.
    const auto snapshot{ store.GetActive() };
    store.SetActive(nullptr);
    EXPECT_EQ(store.GetActive(), nullptr);
    ASSERT_NE(snapshot, nullptr);
    EXPECT_EQ(snapshot->Name, "demo");
}

TEST_F(PresetTests, ResolveReadsTheCompiledFilesFromTheStorageFolderAndChangesNothing)
{
    const auto pick{ m_Service.Resolve("fixture") };

    ASSERT_TRUE(pick.has_value());
    EXPECT_EQ(pick->Name, "fixture");
    ASSERT_NE(pick->Preset, nullptr);
    ASSERT_NE(pick->Catalog, nullptr);
    EXPECT_GT(pick->Catalog->Fields.size(), 900u);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
    EXPECT_TRUE(m_Service.GetActiveName().empty());
}

TEST_F(PresetTests, ResolveOfNoNameIsThePickOfTheBaseValues)
{
    const auto pick{ m_Service.Resolve("") };

    ASSERT_TRUE(pick.has_value());
    EXPECT_EQ(pick->Preset, nullptr);
    EXPECT_TRUE(pick->Name.empty());
}

TEST_F(PresetTests, ResolveFailsWithAReasonAndLogsAnError)
{
    for (const std::string_view bad : { "missing", "Fixture", "../fixture", "has space", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" })
    {
        const auto pick{ m_Service.Resolve(bad) };
        EXPECT_FALSE(pick.has_value()) << bad;
    }
    EXPECT_TRUE(Logged(LogLevel::Error, "\"missing\" not set"));

    WritePreset("broken", "preset broken\nwrite zz load 00 ff\n");
    const auto broken{ m_Service.Resolve("broken") };
    ASSERT_FALSE(broken.has_value());
    EXPECT_NE(broken.error().find("line 2"), std::string::npos);

    WritePreset("other", "preset another\nsource 0\n");
    EXPECT_FALSE(m_Service.Resolve("other").has_value());

    m_SettingsStore.SetAppDataDirectory("");
    EXPECT_FALSE(m_Service.Resolve("fixture").has_value());
}

TEST_F(PresetTests, AdoptMakesThePickTheActivePresetAndAPickWithNoPresetClearsIt)
{
    m_Service.Adopt(this->Pick("fixture"));

    EXPECT_EQ(m_Service.GetActiveName(), "fixture");
    EXPECT_NE(m_Service.GetActive().Preset, nullptr);
    EXPECT_NE(m_Store.GetCatalog(), nullptr);

    m_Service.Adopt(PresetPick{});
    EXPECT_EQ(m_Store.GetActive(), nullptr);
    EXPECT_TRUE(m_Service.GetActive().Name.empty());
}

TEST_F(PresetTests, WriteToStartWritesThePresetIntoTheVariantOfTheCopyAndKeepsTheBase)
{
    const PresetPick pick{ this->Pick("fixture") };
    const std::vector<std::byte> original{ MakeConfiguration() };
    std::vector<std::byte> start{ original };

    const auto written{ m_Service.WriteToStart(start, pick) };

    ASSERT_TRUE(written.has_value());
    EXPECT_TRUE(*written);
    EXPECT_EQ(VariantByte(start, k_VariantRespawnTime), 3);
    EXPECT_EQ(VariantByte(start, k_VariantRoundLimit), 3);
    // Friendly fire is bit 0 of the social flags and the powerups bit of the map flags is cleared, the other bits stay.
    EXPECT_EQ(VariantByte(start, 0x300), VariantByte(original, 0x300) & ~0x08);
    EXPECT_EQ(VariantByte(start, 0x2FC), VariantByte(original, 0x2FC) | 0x01);

    // Only bytes of the variant changed, never the ones before it.
    for (std::size_t i{}; i < k_ConfigurationVariantOffset; ++i) ASSERT_EQ(start[i], original[i]) << i;
    EXPECT_NE(start, original);

    // The base is what the variant held before: respawn time and round limit are the original bytes.
    const auto base{ m_Store.GetBase() };
    ASSERT_NE(base, nullptr);
    EXPECT_FALSE(base->Bytes.empty());
    EXPECT_NE(std::ranges::find(base->Bytes, std::to_integer<std::uint8_t>(original[k_ConfigurationVariantOffset + k_VariantRespawnTime])), base->Bytes.end());
    EXPECT_EQ(m_Store.GetBaseCatalog(), pick.Catalog);
    // Adopting is the business of the caller: the service of the setup does it once the whole setup is in.
    EXPECT_EQ(m_Store.GetActive(), nullptr);
}

TEST_F(PresetTests, WriteToStartOfNoPresetChangesNothing)
{
    const std::vector<std::byte> original{ MakeConfiguration() };
    std::vector<std::byte> start{ original };

    const auto written{ m_Service.WriteToStart(start, PresetPick{}) };

    ASSERT_TRUE(written.has_value());
    EXPECT_FALSE(*written);
    EXPECT_EQ(start, original);
    EXPECT_EQ(m_Store.GetBase(), nullptr);
}

TEST_F(PresetTests, AVariantThatIsNotMegaloGetsNothingWritten)
{
    const PresetPick pick{ this->Pick("fixture") };
    std::vector<std::byte> start{ MakeConfiguration() };
    const std::uint32_t other{ 1 };
    std::memcpy(start.data() + k_ConfigurationVariantOffset + k_VariantEngineType, &other, sizeof(other));
    const std::vector<std::byte> before{ start };

    const auto written{ m_Service.WriteToStart(start, pick) };

    ASSERT_TRUE(written.has_value());
    EXPECT_FALSE(*written);
    EXPECT_EQ(start, before);
    EXPECT_EQ(m_Store.GetBase(), nullptr);
}

TEST_F(PresetTests, ATraitSetTheScriptDoesNotHaveFailsTheWriteAndLeavesTheCopyAsItWas)
{
    WritePreset("rejected", k_RejectedPreset);
    const PresetPick pick{ this->Pick("rejected") };
    std::vector<std::byte> start{ MakeConfiguration() };
    const std::vector<std::byte> before{ start };

    const auto written{ m_Service.WriteToStart(start, pick) };

    ASSERT_FALSE(written.has_value());
    EXPECT_EQ(start, before);
    EXPECT_NE(written.error().find("trait set 5"), std::string::npos);
    EXPECT_EQ(m_Store.GetBase(), nullptr);
}

TEST_F(PresetTests, ACopyTooSmallToHoldAVariantFails)
{
    const PresetPick pick{ this->Pick("fixture") };
    std::vector<std::byte> start(k_ConfigurationVariantOffset, std::byte{ 0x11 });

    const auto written{ m_Service.WriteToStart(start, pick) };

    ASSERT_FALSE(written.has_value());
    EXPECT_NE(written.error().find("too small"), std::string::npos);
    EXPECT_EQ(start, std::vector<std::byte>(k_ConfigurationVariantOffset, std::byte{ 0x11 }));
}

TEST_F(PresetTests, PrepareLiveChangesNothingAndCommitLiveWritesThePlan)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    const auto plan{ m_Service.PrepareLive(this->Pick("between"), variant) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(variant, original);
    EXPECT_EQ(m_Store.GetBase(), nullptr);
    EXPECT_EQ(m_Store.GetActive(), nullptr);

    ASSERT_TRUE(m_Service.CommitLive(*plan, variant).has_value());
    EXPECT_EQ(variant[k_VariantRespawnTime], 3);
    EXPECT_EQ(variant[k_VariantRoundLimit], 3);
    // The store changes only when the whole setup is in.
    EXPECT_EQ(m_Store.GetActive(), nullptr);

    m_Service.AdoptLive(*plan);
    EXPECT_EQ(m_Store.GetActiveName(), "between");
    EXPECT_NE(m_Store.GetBase(), nullptr);
}

TEST_F(PresetTests, TheBaseIsSavedAtTheFirstPresetOfAMatchLoadedWithoutOneAndKeptUntilTheNextLoad)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };
    EXPECT_EQ(m_Store.GetBase(), nullptr);

    ASSERT_TRUE(this->Live(this->Pick("between"), variant).has_value());

    const auto base{ m_Store.GetBase() };
    ASSERT_NE(base, nullptr);
    EXPECT_NE(std::ranges::find(base->Bytes, original[k_VariantRespawnTime]), base->Bytes.end());

    // A second preset keeps the same base.
    ASSERT_TRUE(this->Live(this->Pick("between"), variant).has_value());
    EXPECT_EQ(m_Store.GetBase(), base);

    // The next start drops it, with or without a preset.
    std::vector<std::byte> start{ MakeConfiguration() };
    ASSERT_TRUE(this->Start(start, this->Pick("fixture")).value_or(false));
    EXPECT_NE(m_Store.GetBase(), base);

    ASSERT_TRUE(this->Start(start, PresetPick{}).has_value());
    EXPECT_EQ(m_Store.GetBase(), nullptr);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
}

TEST_F(PresetTests, NoPresetBetweenRoundsRestoresTheBaseAndClearsTheActivePreset)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(this->Live(this->Pick("between"), variant).has_value());
    ASSERT_NE(variant, original);

    ASSERT_TRUE(this->Live(PresetPick{}, variant).has_value());

    EXPECT_EQ(variant, original);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
}

TEST_F(PresetTests, NoPresetWithNothingWrittenLeavesTheVariantAlone)
{
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    const auto plan{ m_Service.PrepareLive(PresetPick{}, variant) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_TRUE(plan->Writes.empty());
    ASSERT_TRUE(this->Live(PresetPick{}, variant).has_value());
    EXPECT_EQ(variant, original);
}

TEST_F(PresetTests, AFieldOnlyALoadCanChangeIsRefusedAndKeepsTheVariantAndTheActivePreset)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };

    ASSERT_TRUE(this->Live(this->Pick("between"), variant).has_value());
    const std::vector<std::uint8_t> applied{ variant };

    // The fixture changes load fields the match was not loaded with.
    const auto result{ this->Live(this->Pick("fixture"), variant) };

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(variant, applied);
    EXPECT_EQ(m_Store.GetActiveName(), "between");
    EXPECT_NE(result.error().find("only change at load"), std::string::npos);
}

TEST_F(PresetTests, ATraitSetTheScriptLacksIsRefusedBetweenRounds)
{
    WritePreset("rejected", k_RejectedPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    const auto result{ this->Live(this->Pick("rejected"), variant) };

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(variant, original);
    EXPECT_NE(result.error().find("trait set 5"), std::string::npos);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
}

TEST_F(PresetTests, AVariantThatIsNotMegaloIsRefusedBetweenRounds)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };
    variant[k_VariantEngineType] = 1;
    const std::vector<std::uint8_t> before{ variant };

    const auto result{ this->Live(this->Pick("between"), variant) };

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(variant, before);
}

TEST_F(PresetTests, EachMatchComposesOverItsOwnBase)
{
    WritePreset("a", "preset a\nsource 0\nwrite 2c7 spawn 03 ff\nwrite 2bf round_end 03 ff\n");
    WritePreset("b", "preset b\nsource 0\nwrite 2bf round_end 04 ff\n");

    // Match 1 has no preset, gets A, then none.
    std::vector<std::byte> first{ MakeConfiguration() };
    ASSERT_TRUE(this->Start(first, PresetPick{}).has_value());
    std::vector<std::uint8_t> live1{ VariantOf(first) };
    const std::vector<std::uint8_t> original1{ live1 };

    ASSERT_TRUE(this->Live(this->Pick("a"), live1).has_value());
    EXPECT_EQ(live1[k_VariantRespawnTime], 3);
    const auto base1{ m_Store.GetBase() };
    ASSERT_NE(base1, nullptr);

    ASSERT_TRUE(this->Live(PresetPick{}, live1).has_value());
    EXPECT_EQ(live1, original1);

    // Match 2 starts with another respawn time and gets B at load: B does not write it, so it stays the one of match 2.
    std::vector<std::byte> second{ MakeConfiguration() };
    second[k_ConfigurationVariantOffset + k_VariantRespawnTime] = std::byte{ 0x55 };
    ASSERT_TRUE(this->Start(second, this->Pick("b")).value_or(false));
    EXPECT_EQ(VariantByte(second, k_VariantRespawnTime), 0x55);
    EXPECT_EQ(VariantByte(second, k_VariantRoundLimit), 4);

    const auto base2{ m_Store.GetBase() };
    ASSERT_NE(base2, nullptr);
    EXPECT_NE(base2, base1);
    EXPECT_NE(std::ranges::find(base2->Bytes, std::uint8_t{ 0x55 }), base2->Bytes.end());

    // Back to none in match 2 gives its own values, not the ones of match 1.
    std::vector<std::uint8_t> live2{ VariantOf(second) };
    ASSERT_TRUE(this->Live(PresetPick{}, live2).has_value());
    EXPECT_EQ(live2[k_VariantRespawnTime], 0x55);
    EXPECT_EQ(live2[k_VariantRoundLimit], VariantByte(MakeConfiguration(), k_VariantRoundLimit));
}

TEST_F(PresetTests, GoingFromAToBGivesBackTheBaseOfAFieldBLeavesOut)
{
    WritePreset("a", "preset a\nsource 0\nwrite 2c7 spawn 03 ff\nwrite 2bf round_end 03 ff\n");
    WritePreset("b", "preset b\nsource 0\nwrite 2bf round_end 04 ff\n");
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::vector<std::uint8_t> original{ variant };

    ASSERT_TRUE(this->Live(this->Pick("a"), variant).has_value());
    ASSERT_EQ(variant[k_VariantRespawnTime], 3);

    ASSERT_TRUE(this->Live(this->Pick("b"), variant).has_value());
    EXPECT_EQ(variant[k_VariantRespawnTime], original[k_VariantRespawnTime]);
    EXPECT_EQ(variant[k_VariantRoundLimit], 4);
}

TEST_F(PresetTests, AFieldOnlyALoadCanChangeIsAcceptedBetweenRoundsWhenItAlreadyHasTheValue)
{
    std::vector<std::uint8_t> variant{ MakeVariant() };
    const std::uint8_t current{ static_cast<std::uint8_t>(variant[0x2C4] & 0x08) };
    WritePreset("loadeq", std::format("preset loadeq\nsource 0\nwrite 2c4 load {:02x} 08\nwrite 2bf round_end 03 ff\n", current));

    const auto result{ this->Live(this->Pick("loadeq"), variant) };

    EXPECT_TRUE(result.has_value()) << result.error_or("");
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

    const auto result{ this->Live(this->Pick("overlap"), variant) };

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(variant, original);
    EXPECT_NE(result.error().find("read back"), std::string::npos);
    EXPECT_EQ(m_Store.GetActive(), nullptr);
}

TEST_F(PresetTests, APickWithAnotherCatalogThanTheBaseIsRefusedWithAReasonByCheckAndByPrepareLive)
{
    WritePreset("between", k_BetweenPreset);
    std::vector<std::uint8_t> variant{ MakeVariant() };

    // Without a base nothing is compared.
    EXPECT_FALSE(m_Service.Check(this->Pick("between")).has_value());

    ASSERT_TRUE(this->Live(this->Pick("between"), variant).has_value());
    const std::vector<std::uint8_t> applied{ variant };
    EXPECT_FALSE(m_Service.Check(this->Pick("between")).has_value());

    {
        std::ofstream catalog{ m_Folder / "Presets" / "_catalog.txt", std::ios::binary | std::ios::app };
        catalog << "\nfield 20 1 ff tick\n";
    }
    const PresetPick other{ this->Pick("between") };

    const auto reason{ m_Service.Check(other) };
    ASSERT_TRUE(reason.has_value());
    EXPECT_NE(reason->find("catalog"), std::string::npos);

    const auto result{ this->Live(other, variant) };
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(variant, applied);
    EXPECT_NE(result.error().find("catalog"), std::string::npos);
    EXPECT_EQ(m_Store.GetActiveName(), "between");
}
