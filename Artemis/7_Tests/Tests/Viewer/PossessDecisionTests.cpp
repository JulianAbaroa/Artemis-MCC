#include <gtest/gtest.h>

import DevTools.Viewer.Roster.System;
import std;

// When the Viewer asks the game to let the user possess a bot (VW.9c.2 B, D352).

namespace
{
    using DevTools::Viewer::Roster::System::DecidePossession;

    constexpr std::uint32_t k_Bot{ 0x1003 };
}

TEST(DecidePossessionTests, GameCameraWithABotSelectedAndTheOptionOnPossessesIt)
{
    EXPECT_EQ(DecidePossession(true, true, k_Bot, true), std::optional<std::uint32_t>{ k_Bot });
}

TEST(DecidePossessionTests, TheOptionOffReleases)
{
    EXPECT_FALSE(DecidePossession(false, true, k_Bot, true).has_value());
}

TEST(DecidePossessionTests, TheFreeCameraReleases)
{
    EXPECT_FALSE(DecidePossession(true, false, k_Bot, true).has_value());
}

TEST(DecidePossessionTests, AHumanSelectedReleases)
{
    EXPECT_FALSE(DecidePossession(true, true, 0x1000, false).has_value());
}

TEST(DecidePossessionTests, NothingSelectedReleases)
{
    EXPECT_FALSE(DecidePossession(true, true, 0xFFFFFFFF, false).has_value());
    EXPECT_FALSE(DecidePossession(true, true, 0xFFFFFFFF, true).has_value());
}

TEST(DecidePossessionTests, ABotThatDiesStaysPossessed)
{
    // note: The selection follows the player, so the same handle keeps being asked while the bot has no body.
    EXPECT_EQ(DecidePossession(true, true, k_Bot, true), std::optional<std::uint32_t>{ k_Bot });
    EXPECT_EQ(DecidePossession(true, true, k_Bot, true), std::optional<std::uint32_t>{ k_Bot });
}
