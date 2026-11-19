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
    using DevTools::Viewer::Options::Type::Scalar;
    using DevTools::Viewer::Options::Type::k_FlagCount;
    using DevTools::Viewer::Options::Type::k_ScalarCount;

    // Keys of options removed by VW.P: a file saved before still has them.
    constexpr std::array<std::string_view, 5> k_RemovedKeys{ "FirstPersonOverOverlay", "FirstPersonMaskShown", "SceneColorByGameHdr",
        "WaitFrameScene", "RecordFrameClock" };

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
    const std::array<std::pair<std::string_view, std::string_view>, 7> lines{ {
        { "HideGameHud", "1" },
        { "FirstPersonOverOverlay", "1" },
        { "SceneColorByGameHdr", "1" },
        { "SceneColorScale", "0.0625" },
        { "RecordFrameClock", "1" },
        { "WaitFrameScene", "0" },
        { "FirstPersonMaskShown", "1" }
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
    store.SetEnabled(Flag::Overlay, true);
    store.SetEnabled(Flag::HideFirstPerson, true);
    store.SetScalar(Scalar::SceneColorScale, 0.000244140625f);

    const std::string saved{ Snapshot(store) };
    EXPECT_EQ(static_cast<std::size_t>(std::ranges::count(saved, '\n')), k_FlagCount + k_ScalarCount);

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
    EXPECT_TRUE(restored.IsEnabled(Flag::Overlay));
    // Saved with 6 significant digits, so the value is the same only up to them.
    EXPECT_NEAR(restored.GetScalar(Scalar::SceneColorScale), 0.000244140625f, 1e-8f);
}