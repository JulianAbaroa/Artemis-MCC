#include <gtest/gtest.h>

import DevTools.Viewer.Selection.State;
import std;

// The selection store of the Viewer, built from its own source: a selection can follow a player (VW.9a B6).

namespace
{
    using DevTools::Viewer::Selection::State::SelectionStore;

    constexpr std::uint32_t k_None{ 0xFFFFFFFF };
    constexpr std::uint32_t k_Object{ 0x00020010 };
    constexpr std::uint32_t k_Biped{ 0x00030011 };
    constexpr std::uint32_t k_NewBiped{ 0x00030022 };
    constexpr std::uint32_t k_Player{ 0x00010003 };
}

TEST(SelectionStoreTests, ThereIsNoSelectionAtFirst)
{
    SelectionStore store{};
    EXPECT_FALSE(store.HasSelection());
    EXPECT_EQ(store.GetSelected(), k_None);
    EXPECT_EQ(store.GetSelectedPlayer(), k_None);
}

TEST(SelectionStoreTests, AnObjectSelectionFollowsNoPlayer)
{
    SelectionStore store{};
    store.Follow(k_Player, k_Biped);

    store.SetSelected(k_Object);
    EXPECT_EQ(store.GetSelected(), k_Object);
    EXPECT_EQ(store.GetSelectedPlayer(), k_None);
}

TEST(SelectionStoreTests, FollowingAPlayerKeepsItWhileTheBipedChanges)
{
    SelectionStore store{};
    store.Follow(k_Player, k_Biped);
    EXPECT_EQ(store.GetSelected(), k_Biped);
    EXPECT_EQ(store.GetSelectedPlayer(), k_Player);

    // Dead without a biped: the player stays selected.
    store.Follow(k_Player, k_None);
    EXPECT_TRUE(store.HasSelection());
    EXPECT_EQ(store.GetSelected(), k_None);
    EXPECT_EQ(store.GetSelectedPlayer(), k_Player);

    // Respawn: the new biped.
    store.Follow(k_Player, k_NewBiped);
    EXPECT_EQ(store.GetSelected(), k_NewBiped);
    EXPECT_EQ(store.GetSelectedPlayer(), k_Player);
}

TEST(SelectionStoreTests, ClearDropsTheObjectAndThePlayer)
{
    SelectionStore store{};
    store.Follow(k_Player, k_Biped);

    store.Clear();
    EXPECT_FALSE(store.HasSelection());
    EXPECT_EQ(store.GetSelected(), k_None);
    EXPECT_EQ(store.GetSelectedPlayer(), k_None);
}