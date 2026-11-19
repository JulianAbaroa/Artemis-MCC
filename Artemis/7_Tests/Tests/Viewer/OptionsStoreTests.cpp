#include <gtest/gtest.h>

import DevTools.Viewer.Options.State;
import DevTools.Viewer.Options.Type;
import std;

// The options store of the Viewer, built from its own sources (they import nothing but std), so the preferences file can be checked without
// the game. A preferences file written by an older build can hold the keys of options that were removed.

namespace
{
    using DevTools::Viewer::Options::State::OptionsStore;
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Options::Type::FlagInfo;
    using DevTools::Viewer::Options::Type::Layer;
    using DevTools::Viewer::Options::Type::Level;
    using DevTools::Viewer::Options::Type::Scalar;
    using DevTools::Viewer::Options::Type::k_Flags;
    using DevTools::Viewer::Options::Type::k_LayerCount;
    using DevTools::Viewer::Options::Type::k_Layers;
    using DevTools::Viewer::Options::Type::k_ScalarCount;

    // Keys of options removed by VW.P: a file saved before still has them.
    constexpr std::array<std::string_view, 10> k_RemovedKeys{ "FirstPersonOverOverlay", "FirstPersonMaskShown", "SceneColorByGameHdr",
        "WaitFrameScene", "RecordFrameClock", "Overlay", "OverlayInScene", "DriveGameCamera", "MapCollision", "GameFov" };

    // return: How many flags are saved by their own key, the ones that belong to no layer.
    auto CountLooseFlags() -> std::size_t
    {
        return static_cast<std::size_t>(std::ranges::count_if(k_Flags, [](const FlagInfo& info) { return info.Owner == Layer::Count; }));
    }

    // Sets the level of every layer.
    auto SetAllLevels(OptionsStore& store, Level level) -> void
    {
        for (const auto& info : k_Layers) store.SetLevel(info.Id, level);
    }

    auto Snapshot(const OptionsStore& store) -> std::string
    {
        std::ostringstream stream{};
        store.Save(stream);
        return stream.str();
    }
}

TEST(OptionsStoreTests, LoadIgnoresTheKeysOfRemovedOptionsAndKeepsTheOthers)
{
    OptionsStore store{};
    store.SetEnabled(Flag::HideGameHud, true);
    store.SetScalar(Scalar::SceneColorScale, 0.5f);
    const std::string before{ Snapshot(store) };

    for (const std::string_view key : k_RemovedKeys)
    {
        store.Load(key, "1");
        store.Load(key, "0");
        store.Load(key, "not a value");
    }

    store.Load("NotAnOption", "1");
    store.Load("", "");

    EXPECT_EQ(Snapshot(store), before);
    EXPECT_TRUE(store.IsEnabled(Flag::HideGameHud));
    EXPECT_FLOAT_EQ(store.GetScalar(Scalar::SceneColorScale), 0.5f);
}

TEST(OptionsStoreTests, AnOldPreferencesFileLoadsLineByLineWithoutTheRemovedKeys)
{
    // The lines of a file from before: the removed keys between options that stay.
    const std::array<std::pair<std::string_view, std::string_view>, 12> lines{ {
        { "HideGameHud", "1" },
        { "FirstPersonOverOverlay", "1" },
        { "SceneColorByGameHdr", "1" },
        { "SceneColorScale", "0.0625" },
        { "RecordFrameClock", "1" },
        { "WaitFrameScene", "0" },
        { "FirstPersonMaskShown", "1" },
        { "Overlay", "1" },
        { "OverlayInScene", "1" },
        { "DriveGameCamera", "1" },
        { "MapCollision", "1" },
        { "GameFov", "90" }
    } };

    OptionsStore store{};

    for (const auto& [key, value] : lines) store.Load(key, value);

    EXPECT_TRUE(store.IsEnabled(Flag::HideGameHud));
    EXPECT_FLOAT_EQ(store.GetScalar(Scalar::SceneColorScale), 0.0625f);

    // What is saved now has none of the removed keys.
    const std::string saved{ Snapshot(store) };

    for (const std::string_view key : k_RemovedKeys)
    {
        EXPECT_EQ(saved.find(std::string{ "Viewer_" } + std::string{ key } + "="), std::string::npos) << key;
    }
}

TEST(OptionsStoreTests, SaveWritesEveryOptionAndLoadRestoresIt)
{
    OptionsStore store{};
    store.SetEnabled(Flag::HideGameHud, true);
    store.SetEnabled(Flag::HideFirstPerson, true);
    store.SetLevel(Layer::Players, Level::Full);
    store.SetLevel(Layer::Health, Level::Off);
    store.SetScalar(Scalar::SceneColorScale, 0.000244140625f);

    const std::string saved{ Snapshot(store) };
    EXPECT_EQ(static_cast<std::size_t>(std::ranges::count(saved, '\n')), k_LayerCount + CountLooseFlags() + k_ScalarCount);

    OptionsStore restored{};
    std::istringstream stream{ saved };
    std::string line{};

    while (std::getline(stream, line))
    {
        const std::size_t separator{ line.find('=') };
        ASSERT_NE(separator, std::string::npos);
        ASSERT_EQ(line.rfind(OptionsStore::k_PreferencePrefix, 0), 0u);

        restored.Load(std::string_view{ line }.substr(OptionsStore::k_PreferencePrefix.size(), separator - OptionsStore::k_PreferencePrefix.size()),
            std::string_view{ line }.substr(separator + 1));
    }

    EXPECT_EQ(Snapshot(restored), saved);
    EXPECT_TRUE(restored.IsEnabled(Flag::HideGameHud));
    EXPECT_EQ(restored.GetLevel(Layer::Players), Level::Full);
    EXPECT_EQ(restored.GetLevel(Layer::Health), Level::Off);
    EXPECT_EQ(restored.GetLevel(Layer::View), Level::Minimal);
    // Saved with 6 significant digits, so the value is the same only up to them.
    EXPECT_NEAR(restored.GetScalar(Scalar::SceneColorScale), 0.000244140625f, 1e-8f);
}

TEST(OptionsStoreTests, EveryLayerAtMinimalGivesTheDefaultsOfBeforeTheLayers)
{
    const OptionsStore store{};

    for (const FlagInfo& info : k_Flags)
    {
        EXPECT_EQ(store.IsEnabled(info.Id), info.Default) << info.Key;
    }
}

TEST(OptionsStoreTests, ALayerOffTurnsItsFlagsOffAndFullAddsTheFullOnesAndShowsEveryObject)
{
    OptionsStore store{};

    SetAllLevels(store, Level::Off);

    for (const FlagInfo& info : k_Flags)
    {
        if (info.Owner != Layer::Count) EXPECT_FALSE(store.IsEnabled(info.Id)) << info.Key;
    }

    SetAllLevels(store, Level::Full);

    for (const FlagInfo& info : k_Flags)
    {
        if (info.Owner == Layer::Count) continue;

        // The selection-only flags are the exception: at Full every object is shown, so they are off.
        EXPECT_EQ(store.IsEnabled(info.Id), !info.IsInverted) << info.Key;
    }

    EXPECT_FALSE(store.IsEnabled(Flag::ObjectSelectedOnly));
    EXPECT_FALSE(store.IsEnabled(Flag::HealthSelectedOnly));
    EXPECT_TRUE(store.IsEnabled(Flag::PlayerDistance));

    // One layer changes only its own flags.
    SetAllLevels(store, Level::Minimal);
    store.SetLevel(Layer::Objects, Level::Full);

    EXPECT_TRUE(store.IsEnabled(Flag::ObjectHandle));
    EXPECT_FALSE(store.IsEnabled(Flag::PlayerDistance));
    EXPECT_TRUE(store.IsEnabled(Flag::HealthSelectedOnly));
}

TEST(OptionsStoreTests, TheFlagsOfALayerCannotBeSetOneByOneAndTheOthersCan)
{
    OptionsStore store{};

    store.SetEnabled(Flag::PlayerGamertag, false);
    store.ToggleEnabled(Flag::ObjectSelectedOnly);
    EXPECT_TRUE(store.IsEnabled(Flag::PlayerGamertag));
    EXPECT_TRUE(store.IsEnabled(Flag::ObjectSelectedOnly));

    store.ToggleEnabled(Flag::Labels);
    EXPECT_FALSE(store.IsEnabled(Flag::Labels));
}

TEST(OptionsStoreTests, TheKeysOfTheFlagsOfALayerAreIgnoredOnLoadAndALevelOutOfRangeFallsBackToItsDefault)
{
    OptionsStore store{};
    store.SetLevel(Layer::Players, Level::Full);
    const std::string before{ Snapshot(store) };

    // Saved by a build from before the layers.
    for (const FlagInfo& info : k_Flags)
    {
        if (info.Owner == Layer::Count) continue;

        store.Load(info.Key, "0");
        store.Load(info.Key, "1");
    }

    EXPECT_EQ(Snapshot(store), before);

    store.Load("LayerPlayers", "7");
    EXPECT_EQ(store.GetLevel(Layer::Players), Level::Minimal);

    store.Load("LayerPlayers", "2");
    EXPECT_EQ(store.GetLevel(Layer::Players), Level::Full);

    store.Load("LayerPlayers", "-1");
    EXPECT_EQ(store.GetLevel(Layer::Players), Level::Minimal);

    store.Load("LayerObjects", "0");
    EXPECT_EQ(store.GetLevel(Layer::Objects), Level::Off);

    store.Load("LayerObjects", "not a level");
    EXPECT_EQ(store.GetLevel(Layer::Objects), Level::Minimal);

    // The flags without a layer keep their old keys.
    store.Load("HideGameHud", "1");
    store.Load("HideFirstPerson", "1");
    EXPECT_TRUE(store.IsEnabled(Flag::HideGameHud));
    EXPECT_TRUE(store.IsEnabled(Flag::HideFirstPerson));
}

TEST(OptionsStoreTests, ResetToDefaultsPutsTheLevelsBack)
{
    OptionsStore store{};
    SetAllLevels(store, Level::Full);
    store.SetEnabled(Flag::HideGameHud, true);

    store.ResetToDefaults();

    for (const auto& info : k_Layers) EXPECT_EQ(store.GetLevel(info.Id), Level::Minimal);
    EXPECT_FALSE(store.IsEnabled(Flag::HideGameHud));
}