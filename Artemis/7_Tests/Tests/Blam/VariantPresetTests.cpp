#include <gtest/gtest.h>

import Blam.Layout.Player.Type;
import Blam.Layout.Game.Type;
import std;

// The pure logic of the gametype presets (Q97, D264, D265): parse of the compiled files, compose, apply with masks, restore and the rejection rule.
// The fixture is what Tools/presets.py gives for Tools/Tests/presets/fixture.xml (test_presets.py checks it byte for byte).

namespace
{
    using Blam::Layout::Game::Type::VariantPreset::CompiledPreset;
    using Blam::Layout::Game::Type::VariantPreset::PresetBase;
    using Blam::Layout::Game::Type::VariantPreset::PresetCatalog;
    using Blam::Layout::Game::Type::VariantPreset::PresetPhase;
    using Blam::Layout::Game::Type::VariantPreset::PresetWhen;
    using Blam::Layout::Game::Type::VariantPreset::PresetWrites;
    using Blam::Layout::Game::Type::VariantPreset::Apply;
    using Blam::Layout::Game::Type::VariantPreset::Compose;
    using Blam::Layout::Game::Type::VariantPreset::ParseCatalog;
    using Blam::Layout::Game::Type::VariantPreset::ParsePreset;
    using Blam::Layout::Game::Type::VariantPreset::Plan;
    using Blam::Layout::Game::Type::VariantPreset::SaveBase;
    using Blam::Layout::Game::Type::VariantPreset::Verify;
    using Blam::Layout::Player::Type::Traits::Decode;
    using Blam::Layout::Player::Type::Traits::Lookup;
    using Blam::Layout::Player::Type::Traits::PlayerTraits;
    using Blam::Layout::Player::Type::Traits::k_MovementSpeed;
    using Blam::Layout::Player::Type::Traits::k_MovementSpeedTable;
    using Blam::Layout::Player::Type::Traits::k_Size;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineType;
    using Blam::Layout::Game::Type::Offset::k_VariantEngineTypeMegalo;
    using Blam::Layout::Game::Type::Offset::k_VariantMegaloTraitSetCount;
    using Blam::Layout::Game::Type::Offset::k_VariantMegaloTraitSets;
    using Blam::Layout::Game::Type::Offset::k_VariantRespawnTime;
    using Blam::Layout::Game::Type::Offset::k_VariantRoundLimit;
    using Blam::Layout::Game::Type::Offset::k_VariantGracePeriod;

    constexpr std::size_t k_VariantSize{ 0x10000 };
    constexpr std::size_t k_BaseTraits{ 0x304 };
    constexpr std::size_t k_SocialFlags{ 0x2FC };
    constexpr std::size_t k_MapFlags{ 0x300 };
    constexpr std::size_t k_RespawnFlags{ 0x2C4 };
    constexpr std::size_t k_TeamsFlags{ 0x2BC };
    constexpr std::size_t k_TeamOnePrimaryColor{ 0x460 };

    using Variant = std::vector<std::uint8_t>;

    // return: The text of a fixture file.
    auto ReadFixture(std::string_view name) -> std::string
    {
        // This file is Artemis/7_Tests/Tests/Blam/VariantPresetTests.cpp.
        const std::filesystem::path source{ std::source_location::current().file_name() };
        std::ifstream file{ source.parent_path() / "Fixtures" / "presets" / name, std::ios::binary };
        return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    }

    auto Catalog() -> PresetCatalog
    {
        auto catalog{ ParseCatalog(ReadFixture("_catalog.txt")) };
        EXPECT_TRUE(catalog.has_value());
        return catalog.value_or(PresetCatalog{});
    }

    auto Fixture() -> CompiledPreset
    {
        auto preset{ ParsePreset(ReadFixture("fixture.txt")) };
        EXPECT_TRUE(preset.has_value());
        return preset.value_or(CompiledPreset{});
    }

    auto Parsed(std::string_view text) -> CompiledPreset
    {
        auto preset{ ParsePreset(text) };
        EXPECT_TRUE(preset.has_value()) << (preset ? "" : preset.error());
        return preset.value_or(CompiledPreset{});
    }

    template <typename T>
    auto Put(Variant& variant, std::size_t offset, T value) -> void
    {
        std::memcpy(variant.data() + offset, &value, sizeof(T));
    }

    // return: A Megalo variant with a recognizable value in every byte of the catalog, 3 trait sets and the flags of the measured CTF.
    auto MakeVariant() -> Variant
    {
        Variant variant(k_VariantSize);
        for (std::size_t i{}; i < variant.size(); ++i) variant[i] = static_cast<std::uint8_t>(0x40 + i % 7);

        Put<std::uint32_t>(variant, k_VariantEngineType, k_VariantEngineTypeMegalo);
        Put<std::uint32_t>(variant, k_VariantMegaloTraitSetCount, 3);
        variant[k_SocialFlags] = 0x1E;
        variant[k_MapFlags] = 0x1F;
        variant[k_RespawnFlags] = 0x00;
        variant[k_TeamsFlags] = 0x00;
        return variant;
    }

    auto BaseOf(const Variant& variant) -> PresetBase
    {
        auto base{ SaveBase(Catalog(), variant) };
        EXPECT_TRUE(base.has_value());
        return base.value_or(PresetBase{});
    }

    // Plans and applies a preset in one step.
    auto Install(Variant& variant, const PresetBase& base, const CompiledPreset* preset, PresetPhase phase) -> bool
    {
        auto writes{ Plan(Catalog(), base, preset, variant, phase) };
        if (!writes) return false;
        EXPECT_TRUE(Apply(variant, *writes));
        EXPECT_TRUE(Verify(variant, *writes));
        return true;
    }

    // return: The offsets whose byte differs between two variants.
    auto Changed(const Variant& before, const Variant& after) -> std::set<std::size_t>
    {
        std::set<std::size_t> offsets{};
        for (std::size_t i{}; i < before.size(); ++i)
        {
            if (before[i] != after[i]) offsets.insert(i);
        }
        return offsets;
    }

    constexpr std::string_view k_QuietPreset{ "preset quiet\nsource 0\nwrite 2c7 spawn 03 ff\nwrite 2bf round_end 03 ff\nwrite 300 round_start 00 08\n" };
}

TEST(VariantPresetTest, ParsesTheSharedFixtureAndItsCatalog)
{
    const PresetCatalog catalog{ Catalog() };
    EXPECT_GT(catalog.Fields.size(), 900u);

    const CompiledPreset preset{ Fixture() };
    EXPECT_EQ(preset.Name, "fixture");
    EXPECT_EQ(preset.Source.size(), 40u);
    EXPECT_EQ(preset.Writes.size(), 16u);
    ASSERT_EQ(preset.TraitSetChecks.size(), 1u);
    EXPECT_EQ(preset.TraitSetChecks[0], 0u);

    const auto& write{ preset.Writes[5] };
    EXPECT_EQ(write.Offset, 0x2C7u);
    EXPECT_EQ(write.Bytes, (std::vector<std::uint8_t>{ 0x03 }));
    EXPECT_EQ(write.Mask, (std::vector<std::uint8_t>{ 0xFF }));

    // Every write of the preset is a field of the catalog.
    const PresetBase base{ BaseOf(MakeVariant()) };
    EXPECT_TRUE(Compose(catalog, base, &preset).has_value());
}

TEST(VariantPresetTest, ALineThatIsWrongInvalidatesTheWholePreset)
{
    const std::string good{ ReadFixture("fixture.txt") };
    ASSERT_TRUE(ParsePreset(good).has_value());

    for (const std::string_view bad : { "write zz load 00 ff", "write 2c7 never 03 ff", "write 2c7 spawn 03 ffff", "write 2c7 spawn 3 ff",
                                         "write 2c7 spawn 03", "check other 1", "check megalo_trait_set x", "unknown 1" })
    {
        const auto result{ ParsePreset(good + "\n" + std::string{ bad } + "\n") };
        ASSERT_FALSE(result.has_value()) << bad;
        EXPECT_NE(result.error().find("Line "), std::string::npos) << bad;
    }

    EXPECT_FALSE(ParsePreset("source 0\nwrite 2c7 spawn 03 ff").has_value());
    EXPECT_FALSE(ParsePreset("preset a\npreset b").has_value());
    EXPECT_FALSE(ParseCatalog("field 2c7 1 ff sometimes").has_value());
    EXPECT_FALSE(ParseCatalog("field 2c7 2 ff load").has_value());
    EXPECT_TRUE(ParseCatalog("\nfield 2c7 1 ff spawn\r\n\r\nfield 2c0 2 ffff round_end").has_value());
}

TEST(VariantPresetTest, TheWritesOfBaseTraitsDecodeToTheValuesOfTheXml)
{
    Variant variant{ MakeVariant() };
    const PresetBase base{ BaseOf(variant) };
    const CompiledPreset preset{ Fixture() };

    ASSERT_TRUE(Install(variant, base, &preset, PresetPhase::Load));

    std::array<std::uint8_t, k_Size> block{};
    std::copy_n(variant.begin() + k_BaseTraits, k_Size, block.begin());
    const PlayerTraits traits{ Decode(block) };
    EXPECT_FLOAT_EQ(traits.Gravity, 0.75f);
    EXPECT_FLOAT_EQ(traits.MovementSpeed, 1.1f);
    EXPECT_FLOAT_EQ(traits.JumpHeight, 1.5f);

    // The trait set 0 got speed 90%, and the child's override of the parent's respawn time won.
    EXPECT_FLOAT_EQ(Lookup(k_MovementSpeedTable, variant[k_VariantMegaloTraitSets + k_MovementSpeed]), 0.9f);
    EXPECT_EQ(variant[k_VariantRespawnTime], 3);
}

TEST(VariantPresetTest, AtLoadOnlyTheBytesOfThePresetChangeAndMasksKeepTheOtherBits)
{
    const Variant original{ MakeVariant() };
    Variant variant{ original };
    const PresetBase base{ BaseOf(variant) };
    const CompiledPreset preset{ Fixture() };

    ASSERT_TRUE(Install(variant, base, &preset, PresetPhase::Load));

    // Friendly fire is bit 0 of the social flags (0x1E -> 0x1F), powerups bit 3 of the map flags (0x1F -> 0x17), the others stay.
    EXPECT_EQ(variant[k_SocialFlags], 0x1F);
    EXPECT_EQ(variant[k_MapFlags], 0x17);
    EXPECT_EQ(variant[k_RespawnFlags], 0x08);
    EXPECT_EQ(variant[k_TeamsFlags], 0x08);

    std::set<std::size_t> allowed{};
    for (const auto& write : preset.Writes)
    {
        for (std::size_t i{}; i < write.Bytes.size(); ++i) allowed.insert(write.Offset + i);
    }
    for (const std::size_t offset : Changed(original, variant)) EXPECT_TRUE(allowed.contains(offset)) << std::hex << offset;
    EXPECT_EQ(variant[k_TeamOnePrimaryColor + 1], 0xFF);
    EXPECT_EQ(variant[k_TeamOnePrimaryColor], 0x00);

    // The base kept the original value of what the preset wrote.
    const PresetBase after{ BaseOf(original) };
    EXPECT_EQ(base.Bytes, after.Bytes);
}

TEST(VariantPresetTest, ANonMegaloVariantIsRejectedAndNothingChanges)
{
    Variant variant{ MakeVariant() };
    const PresetBase base{ BaseOf(variant) };
    Put<std::uint32_t>(variant, k_VariantEngineType, 1);
    const Variant before{ variant };
    const CompiledPreset preset{ Fixture() };

    const auto writes{ Plan(Catalog(), base, &preset, variant, PresetPhase::Load) };
    ASSERT_FALSE(writes.has_value());
    EXPECT_NE(writes.error().Reason.find("Megalo"), std::string::npos);
    EXPECT_EQ(variant, before);
    EXPECT_FALSE(Plan(Catalog(), base, &preset, std::span<const std::uint8_t>{ variant.data(), 0x100 }, PresetPhase::Load).has_value());
}

TEST(VariantPresetTest, BetweenRoundsTheNewPresetRestoresWhatOnlyTheOldOneTouched)
{
    const Variant original{ MakeVariant() };
    Variant variant{ original };
    const PresetBase base{ BaseOf(variant) };
    const CompiledPreset quiet{ Parsed(k_QuietPreset) };
    const CompiledPreset other{ Parsed("preset other\nsource 0\nwrite 2c7 spawn 09 ff\n") };

    ASSERT_TRUE(Install(variant, base, &quiet, PresetPhase::Load));
    EXPECT_EQ(variant[k_VariantRoundLimit], 3);
    EXPECT_EQ(variant[k_MapFlags], 0x17);

    ASSERT_TRUE(Install(variant, base, &other, PresetPhase::BetweenRounds));
    EXPECT_EQ(variant[k_VariantRespawnTime], 9);
    EXPECT_EQ(variant[k_VariantRoundLimit], original[k_VariantRoundLimit]);
    EXPECT_EQ(variant[k_MapFlags], original[k_MapFlags]);
    EXPECT_EQ(Changed(original, variant), (std::set<std::size_t>{ k_VariantRespawnTime }));
}

TEST(VariantPresetTest, NoPresetRestoresTheWholeBase)
{
    const Variant original{ MakeVariant() };
    Variant variant{ original };
    const PresetBase base{ BaseOf(variant) };
    const CompiledPreset quiet{ Parsed(k_QuietPreset) };

    ASSERT_TRUE(Install(variant, base, &quiet, PresetPhase::Load));
    EXPECT_NE(variant, original);
    ASSERT_TRUE(Install(variant, base, nullptr, PresetPhase::BetweenRounds));
    EXPECT_EQ(variant, original);
}

TEST(VariantPresetTest, ALoadFieldThatWouldChangeRejectsTheWholeRequestAndLeavesTheVariantIntact)
{
    Variant variant{ MakeVariant() };
    const PresetBase base{ BaseOf(variant) };
    const CompiledPreset quiet{ Parsed(k_QuietPreset) };
    ASSERT_TRUE(Install(variant, base, &quiet, PresetPhase::Load));
    const Variant before{ variant };

    // The preset changes the grace period, a field of Load, and also a field that could change between rounds.
    const CompiledPreset load{ Parsed("preset load\nsource 0\nwrite 2c7 spawn 09 ff\nwrite 2c2 load 0500 ffff\n") };
    const auto writes{ Plan(Catalog(), base, &load, variant, PresetPhase::BetweenRounds) };
    ASSERT_FALSE(writes.has_value());
    EXPECT_NE(writes.error().Reason.find("load"), std::string::npos);
    EXPECT_EQ(variant, before);

    // Restoring the base is rejected too when the old preset changed a Load field.
    const CompiledPreset colors{ Parsed("preset colors\nsource 0\nwrite 460 load 00ff0000 ffffffff\n") };
    ASSERT_TRUE(Install(variant, base, &colors, PresetPhase::Load));
    EXPECT_FALSE(Plan(Catalog(), base, nullptr, variant, PresetPhase::BetweenRounds).has_value());
    EXPECT_FALSE(Plan(Catalog(), base, &quiet, variant, PresetPhase::BetweenRounds).has_value());

    // At load the same change is accepted.
    EXPECT_TRUE(Plan(Catalog(), base, &load, variant, PresetPhase::Load).has_value());
}

TEST(VariantPresetTest, ALoadFieldThatKeepsItsValueIsAccepted)
{
    Variant variant{ MakeVariant() };
    const PresetBase base{ BaseOf(variant) };
    Put<std::uint16_t>(variant, k_VariantGracePeriod, 5);
    const CompiledPreset load{ Parsed("preset load\nsource 0\nwrite 2c7 spawn 09 ff\nwrite 2c2 load 0500 ffff\n") };

    const auto writes{ Plan(Catalog(), base, &load, variant, PresetPhase::BetweenRounds) };
    ASSERT_TRUE(writes.has_value());
    // The grace period already holds the value of the preset, so only the respawn time is left to write.
    EXPECT_TRUE(std::ranges::none_of(*writes, [](const auto& write) { return write.Offset == k_VariantGracePeriod; }));
    EXPECT_TRUE(std::ranges::any_of(*writes, [](const auto& write) { return write.Offset == k_VariantRespawnTime; }));
}

TEST(VariantPresetTest, ATraitSetOfThePresetThatTheScriptDoesNotHaveIsRejected)
{
    Variant variant{ MakeVariant() };
    const PresetBase base{ BaseOf(variant) };
    const Variant before{ variant };
    const CompiledPreset three{ Parsed("preset a\nsource 0\ncheck megalo_trait_set 2\n") };
    const CompiledPreset outside{ Parsed("preset b\nsource 0\ncheck megalo_trait_set 3\n") };

    EXPECT_TRUE(Plan(Catalog(), base, &three, variant, PresetPhase::Load).has_value());
    const auto writes{ Plan(Catalog(), base, &outside, variant, PresetPhase::Load) };
    ASSERT_FALSE(writes.has_value());
    EXPECT_NE(writes.error().Reason.find("Trait set 3"), std::string::npos);
    EXPECT_EQ(variant, before);
}

TEST(VariantPresetTest, AWriteThatIsNotAFieldOfTheCatalogIsRejected)
{
    Variant variant{ MakeVariant() };
    const PresetBase base{ BaseOf(variant) };
    const CompiledPreset stray{ Parsed("preset stray\nsource 0\nwrite 2c7 spawn 03 0f\n") };

    const auto writes{ Plan(Catalog(), base, &stray, variant, PresetPhase::Load) };
    ASSERT_FALSE(writes.has_value());
    EXPECT_NE(writes.error().Reason.find("not a field"), std::string::npos);
}

TEST(VariantPresetTest, ApplyWritesNothingWhenAWriteIsOutsideTheVariant)
{
    Variant variant(0x40, 0xAA);
    PresetWrites writes{};
    writes.push_back({ .Offset = 0x10, .When = PresetWhen::Tick, .Bytes = { 0x01 }, .Mask = { 0xFF } });
    writes.push_back({ .Offset = 0x3F, .When = PresetWhen::Tick, .Bytes = { 0x01, 0x02 }, .Mask = { 0xFF, 0xFF } });
    const Variant before{ variant };

    EXPECT_FALSE(Apply(variant, writes));
    EXPECT_EQ(variant, before);
    EXPECT_FALSE(Verify(variant, writes));
}