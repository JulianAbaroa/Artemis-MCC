#include <gtest/gtest.h>

import Common.Team.Type;
import DevTools.Viewer.Roster.Type;
import DevTools.Viewer.Roster.System;
import std;

// The pure ordering and stepping of the arrow roster of the Viewer (VW.9b), built from its own source.

namespace
{
    using Common::Team::Type::Team;
    using DevTools::Viewer::Roster::System::IndexOfPlayer;
    using DevTools::Viewer::Roster::System::KeepAgents;
    using DevTools::Viewer::Roster::System::SortRoster;
    using DevTools::Viewer::Roster::System::StepFrom;
    using DevTools::Viewer::Roster::System::StripOf;
    using DevTools::Viewer::Roster::Type::RosterEntry;

    auto Human(std::uint32_t handle, std::string name, Team team) -> RosterEntry
    {
        return RosterEntry{ .Handle = handle, .Name = std::move(name), .Team = team };
    }

    auto Bot(std::uint32_t handle, std::size_t number, Team team) -> RosterEntry
    {
        return RosterEntry{ .Handle = handle, .Name = std::format("Mini-Place {}", number), .Team = team, .BotNumber = number };
    }

    auto HandlesOf(const std::vector<RosterEntry>& roster) -> std::vector<std::uint32_t>
    {
        std::vector<std::uint32_t> handles{};
        for (const RosterEntry& entry : roster) handles.push_back(entry.Handle);
        return handles;
    }
}

TEST(RosterOrderTests, TeamsGoInEnumOrderWithNeutralLast)
{
    std::vector<RosterEntry> roster{ Human(1, "a", Team::Neutral), Human(2, "a", Team::Blue), Human(3, "a", Team::Red) };
    SortRoster(roster);
    EXPECT_EQ(HandlesOf(roster), (std::vector<std::uint32_t>{ 3, 2, 1 }));
}

TEST(RosterOrderTests, InsideATeamHumansGoByNameThenBotsByNumber)
{
    std::vector<RosterEntry> roster{ Bot(1, 10, Team::Red), Bot(2, 2, Team::Red), Human(3, "Zed", Team::Red), Human(4, "Amy", Team::Red),
        Bot(5, 1, Team::Blue), Human(6, "Bob", Team::Blue) };
    SortRoster(roster);
    EXPECT_EQ(HandlesOf(roster), (std::vector<std::uint32_t>{ 4, 3, 2, 1, 6, 5 }));
}

TEST(RosterOrderTests, TiesGoByHandle)
{
    std::vector<RosterEntry> roster{ Human(9, "Same", Team::Red), Human(4, "Same", Team::Red), Bot(8, 1, Team::Red), Bot(3, 1, Team::Red) };
    SortRoster(roster);
    EXPECT_EQ(HandlesOf(roster), (std::vector<std::uint32_t>{ 4, 9, 3, 8 }));
}

TEST(RosterOrderTests, IndexOfPlayerFindsTheHandleOrNothing)
{
    const std::vector<RosterEntry> roster{ Human(5, "a", Team::Red), Human(7, "b", Team::Red) };
    EXPECT_EQ(IndexOfPlayer(roster, 7), std::optional<std::size_t>{ 1 });
    EXPECT_FALSE(IndexOfPlayer(roster, 99).has_value());
}

TEST(RosterOrderTests, StepWrapsAtBothEnds)
{
    EXPECT_EQ(StepFrom(3, 0, 1), std::optional<std::size_t>{ 1 });
    EXPECT_EQ(StepFrom(3, 2, 1), std::optional<std::size_t>{ 0 });
    EXPECT_EQ(StepFrom(3, 0, -1), std::optional<std::size_t>{ 2 });
    EXPECT_EQ(StepFrom(3, 1, -1), std::optional<std::size_t>{ 0 });
    EXPECT_EQ(StepFrom(3, 1, 4), std::optional<std::size_t>{ 2 });
    EXPECT_EQ(StepFrom(3, 1, -5), std::optional<std::size_t>{ 2 });
}

TEST(RosterOrderTests, WithoutACurrentPlayerRightGoesFirstAndLeftGoesLast)
{
    EXPECT_EQ(StepFrom(4, std::nullopt, 1), std::optional<std::size_t>{ 0 });
    EXPECT_EQ(StepFrom(4, std::nullopt, -1), std::optional<std::size_t>{ 3 });
}

TEST(RosterOrderTests, AnEmptyRosterOrAZeroStepGivesNothingNew)
{
    EXPECT_FALSE(StepFrom(0, std::nullopt, 1).has_value());
    EXPECT_FALSE(StepFrom(0, 0, -1).has_value());
    EXPECT_FALSE(StepFrom(3, std::nullopt, 0).has_value());
    EXPECT_EQ(StepFrom(3, 2, 0), std::optional<std::size_t>{ 2 });
}

TEST(RosterOrderTests, OnePlayerHasNoNeighboursAndStepsOntoItself)
{
    const auto strip{ StripOf(1, 0) };
    EXPECT_FALSE(strip.Previous.has_value());
    EXPECT_FALSE(strip.Next.has_value());
    EXPECT_EQ(strip.Current, 0u);
    EXPECT_EQ(StepFrom(1, 0, 1), std::optional<std::size_t>{ 0 });
}

TEST(RosterOrderTests, TwoPlayersShowTheOtherOnlyAsTheNext)
{
    const auto strip{ StripOf(2, 1) };
    EXPECT_FALSE(strip.Previous.has_value());
    EXPECT_EQ(strip.Next, std::optional<std::size_t>{ 0 });
}

TEST(RosterOrderTests, ThreeOrMorePlayersShowBothNeighboursWrapping)
{
    const auto first{ StripOf(3, 0) };
    EXPECT_EQ(first.Previous, std::optional<std::size_t>{ 2 });
    EXPECT_EQ(first.Next, std::optional<std::size_t>{ 1 });

    const auto last{ StripOf(3, 2) };
    EXPECT_EQ(last.Previous, std::optional<std::size_t>{ 1 });
    EXPECT_EQ(last.Next, std::optional<std::size_t>{ 0 });
}

TEST(RosterOrderTests, KeepAgentsDropsHumansAndKeepsTheOrder)
{
    std::vector<RosterEntry> roster{ Human(1, "a", Team::Green), Bot(2, 0, Team::Red), Human(3, "b", Team::Blue), Bot(4, 1, Team::Blue) };
    KeepAgents(roster);
    EXPECT_EQ(HandlesOf(roster), (std::vector<std::uint32_t>{ 2, 4 }));

    // Nobody left: an empty roster, and the arrows have nothing to do.
    std::vector<RosterEntry> humans{ Human(1, "a", Team::Green) };
    KeepAgents(humans);
    EXPECT_TRUE(humans.empty());
    EXPECT_EQ(StepFrom(humans.size(), std::nullopt, 1), std::nullopt);

    // A selected human is not in the roster: right gives the first agent and left the last.
    EXPECT_EQ(StepFrom(roster.size(), IndexOfPlayer(roster, 1), 1), std::optional<std::size_t>{ 0 });
    EXPECT_EQ(StepFrom(roster.size(), IndexOfPlayer(roster, 1), -1), std::optional<std::size_t>{ 1 });
}