#include <gtest/gtest.h>

import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Perception.Details.Type;
import Perception.Players.Type;
import Views.View.Type;
import Views.View.System;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Roster.System;
import DevTools.Viewer.Roster.Type;
import std;

// The two arrows of the observation of an agent (VW.17), as the pure functions the Viewer layer draws them from. They mirror ObserveBatch in ctf.py.

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::StandDetail;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Views::View::Type::ObsTargets;
    using Views::View::Type::View;
    using Views::View::System::ObsTargetsOf;
    using Views::View::System::RivalTeamOf;
    using DevTools::Viewer::Options::State::OptionsStore;
    using DevTools::Viewer::Options::Type::DecideAllViewsWanted;
    using DevTools::Viewer::Options::Type::DecideScenePasses;
    using DevTools::Viewer::Options::Type::IsPresentationLayer;
    using DevTools::Viewer::Options::Type::Layer;
    using DevTools::Viewer::Options::Type::Level;
    using DevTools::Viewer::Options::Type::ScenePasses;
    using DevTools::Viewer::Options::Type::k_Layers;
    using DevTools::Viewer::Roster::System::AgentRowsOf;
    using DevTools::Viewer::Roster::Type::RosterEntry;

    constexpr Vec3 k_Feet{ 12.0f, -7.0f, 1.5f };
    constexpr Vec3 k_EnemyFlag{ 40.0f, 25.0f, 3.0f };
    constexpr Vec3 k_OwnStand{ -30.0f, 10.0f, 0.5f };

    // A View of a red agent on the red side, with a body, both flags and its own stand.
    auto SidedView() -> View
    {
        View view{};
        view.Head.IsComputed = true;
        view.Head.HasBody = true;
        view.Head.Team = Team::Red;
        view.Head.Side = Team::Red;
        view.Self.Feet = k_Feet;
        view.Flags[0].HasFlag = true;
        view.Flags[0].StandPosition = k_OwnStand;
        view.Flags[1].HasFlag = true;
        view.Flags[1].Position = k_EnemyFlag;
        return view;
    }

    auto IsAt(const Vec3& a, const Vec3& b) -> bool
    {
        return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
    }

    auto StandOf(std::uint32_t handle, Team baseTeam, Team ownerTeam) -> std::pair<std::uint32_t, StandDetail>
    {
        return { handle, StandDetail{ .Handle = handle, .BaseTeam = baseTeam, .OwnerTeam = ownerTeam } };
    }
}

TEST(ObsTargetsTests, AViewThatIsNotComputedHasNoArrows)
{
    View view{ SidedView() };
    view.Head.IsComputed = false;

    const ObsTargets targets{ ObsTargetsOf(view) };

    EXPECT_FALSE(targets.HasFlag);
    EXPECT_FALSE(targets.HasStand);
}

TEST(ObsTargetsTests, AViewWithoutABodyHasNoArrows)
{
    View view{ SidedView() };
    view.Head.HasBody = false;

    const ObsTargets targets{ ObsTargetsOf(view) };

    EXPECT_FALSE(targets.HasFlag);
    EXPECT_FALSE(targets.HasStand);
}

TEST(ObsTargetsTests, BothArrowsStartAtTheFeetAndEndAtTheEnemyFlagAndTheOwnStand)
{
    const ObsTargets targets{ ObsTargetsOf(SidedView()) };

    EXPECT_TRUE(targets.HasFlag);
    EXPECT_TRUE(targets.HasStand);
    EXPECT_TRUE(IsAt(targets.From, k_Feet));
    EXPECT_TRUE(IsAt(targets.Flag, k_EnemyFlag));
    EXPECT_TRUE(IsAt(targets.Stand, k_OwnStand));
}

TEST(ObsTargetsTests, WithoutTheEnemyFlagOnlyTheStandIsDrawn)
{
    View view{ SidedView() };
    view.Flags[1].HasFlag = false;
    view.Flags[1].Position = Vec3{};

    const ObsTargets targets{ ObsTargetsOf(view) };

    EXPECT_FALSE(targets.HasFlag);
    EXPECT_TRUE(targets.HasStand);
    EXPECT_TRUE(IsAt(targets.Stand, k_OwnStand));
}

TEST(ObsTargetsTests, ATeamWithNoSideHasNeitherArrowButKeepsTheFeet)
{
    View view{ SidedView() };
    view.Head.Side = Team::Neutral;

    const ObsTargets targets{ ObsTargetsOf(view) };

    EXPECT_FALSE(targets.HasFlag);
    EXPECT_FALSE(targets.HasStand);
    EXPECT_TRUE(IsAt(targets.From, k_Feet));
}

TEST(ObsTargetsTests, AStandAtZeroIsNoStand)
{
    View view{ SidedView() };
    view.Flags[0].StandPosition = Vec3{};

    const ObsTargets targets{ ObsTargetsOf(view) };

    EXPECT_TRUE(targets.HasFlag);
    EXPECT_FALSE(targets.HasStand);
}

TEST(ObsTargetsTests, TheStandOfTheOwnFlagCountsEvenWhileTheOwnFlagIsRecreated)
{
    View view{ SidedView() };
    view.Flags[0].HasFlag = false;

    const ObsTargets targets{ ObsTargetsOf(view) };

    EXPECT_TRUE(targets.HasStand);
    EXPECT_TRUE(IsAt(targets.Stand, k_OwnStand));
}

TEST(ObsTargetsTests, ACarriedEnemyFlagIsWhereItsCarrierIs)
{
    View view{ SidedView() };
    const Vec3 carrier{ 55.0f, -3.0f, 1.0f };
    view.Flags[1].State = Common::Semantic::Type::FlagState::Carried;
    view.Flags[1].Position = carrier;

    const ObsTargets targets{ ObsTargetsOf(view) };

    EXPECT_TRUE(targets.HasFlag);
    EXPECT_TRUE(IsAt(targets.Flag, carrier));
}

TEST(ObsTargetsTests, TheRivalTeamOwnsTheOtherStand)
{
    ObjectiveDetails objectives{};
    objectives.Stands.insert(StandOf(10, Team::Red, Team::Blue));
    objectives.Stands.insert(StandOf(11, Team::Blue, Team::Red));

    EXPECT_EQ(RivalTeamOf(objectives, Team::Red), Team::Blue);
    EXPECT_EQ(RivalTeamOf(objectives, Team::Blue), Team::Red);
}

TEST(ObsTargetsTests, ATeamWithNoSideOrNoRivalStandHasNoRivalTeam)
{
    ObjectiveDetails objectives{};
    EXPECT_EQ(RivalTeamOf(objectives, Team::Red), Team::Neutral);

    objectives.Stands.insert(StandOf(10, Team::Red, Team::Red));
    EXPECT_EQ(RivalTeamOf(objectives, Team::Red), Team::Neutral);
    EXPECT_EQ(RivalTeamOf(objectives, Team::Neutral), Team::Neutral);
}

TEST(ObsTargetsTests, OnlyTheBotsOfTheRosterAreAgentRows)
{
    Players players{};
    for (const std::uint32_t handle : { 100u, 200u, 300u, 400u })
    {
        Player player{};
        player.Handle = handle;
        players.Rows.push_back(player);
    }

    // 100 is the local human (no bot number), 300 is not in the roster at all.
    const std::vector<RosterEntry> roster{
        RosterEntry{ .Handle = 100 },
        RosterEntry{ .Handle = 200, .BotNumber = 1 },
        RosterEntry{ .Handle = 400, .BotNumber = 2 } };

    EXPECT_EQ(AgentRowsOf(roster, players), (std::vector<std::size_t>{ 1, 3 }));
    EXPECT_TRUE(AgentRowsOf({}, players).empty());
}

TEST(ObsTargetsTests, TheObservationLayerStartsOffAndIsSavedByItsKey)
{
    const auto& info{ k_Layers[static_cast<std::size_t>(Layer::Obs)] };
    EXPECT_STREQ(info.Key, "LayerObs");
    EXPECT_EQ(info.Default, Level::Off);

    OptionsStore store{};
    EXPECT_EQ(store.GetLevel(Layer::Obs), Level::Off);

    store.SetLevel(Layer::Obs, Level::Full);
    std::ostringstream saved{};
    store.Save(saved);
    EXPECT_NE(saved.str().find("Viewer_LayerObs=2\n"), std::string::npos);

    // A value out of range, or text, goes back to the default (Off, unlike the other layers).
    OptionsStore restored{};
    restored.Load("LayerObs", "1");
    EXPECT_EQ(restored.GetLevel(Layer::Obs), Level::Minimal);
    restored.Load("LayerObs", "9");
    EXPECT_EQ(restored.GetLevel(Layer::Obs), Level::Off);
    restored.Load("LayerObs", "2");
    restored.Load("LayerObs", "x");
    EXPECT_EQ(restored.GetLevel(Layer::Obs), Level::Off);
}

TEST(ObsTargetsTests, OnlyTheObservationAndTheCardLayersArePresentationOnes)
{
    for (const auto& info : k_Layers)
    {
        const bool isPresentation{ info.Id == Layer::Obs || info.Id == Layer::Card };
        EXPECT_EQ(IsPresentationLayer(info.Id), isPresentation) << info.Key;
    }
}

TEST(ObsTargetsTests, TheMasterSwitchAndTheObservationLayerDecideTheScenePasses)
{
    // Master on, observation off: today's debug overlay and its mask.
    const ScenePasses debugOnly{ DecideScenePasses(true, Level::Off) };
    EXPECT_TRUE(debugOnly.DrawDebug);
    EXPECT_FALSE(debugOnly.DrawObs);
    EXPECT_FALSE(debugOnly.NeedMapDepth);
    EXPECT_TRUE(debugOnly.NeedMask);

    // Master on and observation on: the debug overlay draws the map itself, so no extra depth pass.
    const ScenePasses both{ DecideScenePasses(true, Level::Minimal) };
    EXPECT_TRUE(both.DrawDebug);
    EXPECT_TRUE(both.DrawObs);
    EXPECT_FALSE(both.NeedMapDepth);
    EXPECT_TRUE(both.NeedMask);

    // Master off and observation on: the presentation path, with the depth of the map and the mask.
    for (const Level level : { Level::Minimal, Level::Full })
    {
        const ScenePasses presentation{ DecideScenePasses(false, level) };
        EXPECT_FALSE(presentation.DrawDebug);
        EXPECT_TRUE(presentation.DrawObs);
        EXPECT_TRUE(presentation.NeedMapDepth);
        EXPECT_TRUE(presentation.NeedMask);
    }

    // Master off and observation off: nothing runs.
    const ScenePasses none{ DecideScenePasses(false, Level::Off) };
    EXPECT_FALSE(none.DrawDebug);
    EXPECT_FALSE(none.DrawObs);
    EXPECT_FALSE(none.NeedMapDepth);
    EXPECT_FALSE(none.NeedMask);
}

TEST(ObsTargetsTests, TheViewerWantsAllViewsOnlyActiveWithoutFocusAndWithALayerOn)
{
    // Active, no focus, either layer on: yes.
    EXPECT_TRUE(DecideAllViewsWanted(true, false, Level::Minimal, Level::Off));
    EXPECT_TRUE(DecideAllViewsWanted(true, false, Level::Off, Level::Full));
    EXPECT_TRUE(DecideAllViewsWanted(true, false, Level::Full, Level::Minimal));

    // Both layers off, a focus, or an inactive Viewer: no.
    EXPECT_FALSE(DecideAllViewsWanted(true, false, Level::Off, Level::Off));
    EXPECT_FALSE(DecideAllViewsWanted(true, true, Level::Full, Level::Full));
    EXPECT_FALSE(DecideAllViewsWanted(false, false, Level::Full, Level::Full));
}