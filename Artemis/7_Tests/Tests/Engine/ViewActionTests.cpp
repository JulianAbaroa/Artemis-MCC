#include <gtest/gtest.h>

import Blam.Layout.Action.Type;
import Common.Engine.Type;
import Engine.Control.Input.System;
import std;

// The decision of which action goes into the input of a view that shows a non local player (VW.9a.y, D349). No game needed.

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Engine::Control::Input::System::FindViewActionPlayer;

    constexpr std::size_t k_Players{ Blam::Layout::Action::Type::Size::k_MaxActionPlayers };

    constexpr std::int32_t k_NotLocal{ -1 };

    // Players 0 and 1 are local (input users 0 and 1), the rest are not.
    auto MakeInputUsers() -> std::array<std::int32_t, k_Players>
    {
        std::array<std::int32_t, k_Players> inputUsers{};
        inputUsers.fill(k_NotLocal);
        inputUsers[0] = 0;
        inputUsers[1] = 1;
        return inputUsers;
    }
}

TEST(ViewActionTests, AViewWithoutPlayerCopiesNothing)
{
    EXPECT_FALSE(FindViewActionPlayer(k_NoHandle, 0xFFFF, MakeInputUsers(), false).has_value());
}

TEST(ViewActionTests, AViewOfALocalPlayerCopiesNothing)
{
    EXPECT_FALSE(FindViewActionPlayer(0, 0xFFFF, MakeInputUsers(), false).has_value());
    EXPECT_FALSE(FindViewActionPlayer(1, 0xFFFF, MakeInputUsers(), false).has_value());
}

TEST(ViewActionTests, AViewOfABotWithActionThisTickCopiesItsAction)
{
    const std::optional<std::size_t> player{ FindViewActionPlayer(5, 1u << 5, MakeInputUsers(), false) };
    ASSERT_TRUE(player.has_value());
    EXPECT_EQ(*player, 5u);
}

TEST(ViewActionTests, AViewOfABotWithoutActionThisTickCopiesNothing)
{
    EXPECT_FALSE(FindViewActionPlayer(5, 0xFFFFu & ~(1u << 5), MakeInputUsers(), false).has_value());
}

TEST(ViewActionTests, TheTheaterCopiesNothingBecauseTheEngineDoesIt)
{
    EXPECT_FALSE(FindViewActionPlayer(5, 0xFFFF, MakeInputUsers(), true).has_value());
}

TEST(ViewActionTests, TwoViewsOnlyTheOneOfTheBotCopies)
{
    const auto inputUsers{ MakeInputUsers() };
    const std::uint32_t mask{ (1u << 0) | (1u << 7) };

    EXPECT_FALSE(FindViewActionPlayer(0, mask, inputUsers, false).has_value());
    const std::optional<std::size_t> bot{ FindViewActionPlayer(7, mask, inputUsers, false) };
    ASSERT_TRUE(bot.has_value());
    EXPECT_EQ(*bot, 7u);
}

TEST(ViewActionTests, TheIndexIsTheLowHalfOfTheHandleAndTheSaltIsIgnored)
{
    const std::uint32_t handle{ (0x1234u << 16) | 9u };
    const std::optional<std::size_t> player{ FindViewActionPlayer(handle, 1u << 9, MakeInputUsers(), false) };
    ASSERT_TRUE(player.has_value());
    EXPECT_EQ(*player, 9u);

    const std::uint32_t localWithSalt{ (0x1234u << 16) | 1u };
    EXPECT_FALSE(FindViewActionPlayer(localWithSalt, 0xFFFF, MakeInputUsers(), false).has_value());
}

TEST(ViewActionTests, AnIndexBeyondTheTableCopiesNothing)
{
    EXPECT_FALSE(FindViewActionPlayer(16, 0xFFFFFFFF, MakeInputUsers(), false).has_value());
    EXPECT_FALSE(FindViewActionPlayer(0x0001FFFF, 0xFFFFFFFF, MakeInputUsers(), false).has_value());
}