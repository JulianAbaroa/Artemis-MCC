#include <gtest/gtest.h>

import Services.Settings.State;
import std;

// Services.Settings: the every-tick demand is a counter of tokens, and each token lowers it at most once.

namespace
{
    using Services::Settings::State::EveryTickDemandToken;
    using Services::Settings::State::SettingsStore;
}

TEST(SettingsStoreTests, EveryTickDemandStartsAtZero)
{
    SettingsStore store{};
    EXPECT_EQ(store.GetEveryTickDemand(), 0u);
}

TEST(SettingsStoreTests, EachTokenRaisesTheDemandAndLowersItWhenDestroyed)
{
    SettingsStore store{};
    {
        EveryTickDemandToken first{ store.RaiseEveryTickDemand() };
        EXPECT_EQ(store.GetEveryTickDemand(), 1u);
        {
            EveryTickDemandToken second{ store.RaiseEveryTickDemand() };
            EXPECT_EQ(store.GetEveryTickDemand(), 2u);
        }

        EXPECT_EQ(store.GetEveryTickDemand(), 1u);
    }

    EXPECT_EQ(store.GetEveryTickDemand(), 0u);
}

TEST(SettingsStoreTests, ResetLowersOnceAndASecondResetDoesNothing)
{
    SettingsStore store{};
    EveryTickDemandToken first{ store.RaiseEveryTickDemand() };
    EveryTickDemandToken second{ store.RaiseEveryTickDemand() };

    first.Reset();
    EXPECT_FALSE(first.IsHeld());
    EXPECT_EQ(store.GetEveryTickDemand(), 1u);

    // The reset of a token that holds nothing must not take the demand of another one.
    first.Reset();
    EXPECT_EQ(store.GetEveryTickDemand(), 1u);
    EXPECT_TRUE(second.IsHeld());

    second.Reset();
    EXPECT_EQ(store.GetEveryTickDemand(), 0u);
}

TEST(SettingsStoreTests, MovingATokenMovesTheDemandWithoutLoweringIt)
{
    SettingsStore store{};
    EveryTickDemandToken first{ store.RaiseEveryTickDemand() };

    EveryTickDemandToken second{ std::move(first) };
    EXPECT_FALSE(first.IsHeld());
    EXPECT_TRUE(second.IsHeld());
    EXPECT_EQ(store.GetEveryTickDemand(), 1u);

    EveryTickDemandToken third{};
    third = std::move(second);
    EXPECT_FALSE(second.IsHeld());
    EXPECT_EQ(store.GetEveryTickDemand(), 1u);

    third.Reset();
    EXPECT_EQ(store.GetEveryTickDemand(), 0u);
}

TEST(SettingsStoreTests, AssigningAnotherTokenLowersTheDemandTheTargetHeld)
{
    SettingsStore store{};
    EveryTickDemandToken held{ store.RaiseEveryTickDemand() };

    held = store.RaiseEveryTickDemand();
    EXPECT_TRUE(held.IsHeld());
    EXPECT_EQ(store.GetEveryTickDemand(), 1u);
}