#include <gtest/gtest.h>

import Services.Observed.State;
import std;

// Services.Observed: what the Viewer asks of the AI thread (D307, D385).

namespace
{
    using Services::Observed::State::ObservedPlayerStore;
}

TEST(ObservedPlayerStoreTests, StartsWithNobodyObservedAndNoWishForAllViews)
{
    const ObservedPlayerStore store{};
    EXPECT_EQ(store.Get(), 0xFFFFFFFFu);
    EXPECT_FALSE(store.IsAllViewsWanted());
}

TEST(ObservedPlayerStoreTests, TheWishForAllViewsIsSetAndClearedApartFromThePlayer)
{
    ObservedPlayerStore store{};
    store.SetAllViewsWanted(true);
    EXPECT_TRUE(store.IsAllViewsWanted());
    EXPECT_EQ(store.Get(), 0xFFFFFFFFu);

    store.Set(0x12340003u);
    store.SetAllViewsWanted(false);
    EXPECT_FALSE(store.IsAllViewsWanted());
    EXPECT_EQ(store.Get(), 0x12340003u);
}