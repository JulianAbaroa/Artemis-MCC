#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Entities.Type;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;
import Views.Raycast.State;
import Views.Raycast.Type;
import Views.View.Type;
import Views.View.System;
import Tests.Support.Pipeline;
import Tests.Support.ViewScene;
import std;

// The rays of each agent (F4.2): the aim hit and the perception rays of a View, cast through the one scene of the tick with the exclusion of
// each player, over made-up objects (planes the rays cross).

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Function;
    using Common::Team::Type::Team;
    using Blam::MapModel::Definitions::Type::Bipd::Bipd;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Perception::ObjectGraph::Type::ObjectNode;
    using Perception::ObjectGraph::Type::ObjectNodes;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::ViewScene::MakeEntity;
    using Tests::Support::ViewScene::MakeObject;
    using Tests::Support::ViewScene::MakePlane;
    using Views::Raycast::Type::HitKind;
    using Views::View::System::FindObservedView;
    using Views::View::System::RaycastsOf;
    using Views::View::Type::AgentSet;
    using Views::View::Type::View;
    using Views::View::Type::ViewRelation;
    using Views::View::Type::ViewTable;

    using Common::Engine::Type::k_NoHandle;

    constexpr std::uint32_t k_PlayerA{ 0xE5A10001 };
    constexpr std::uint32_t k_PlayerB{ 0xE5A20002 };
    constexpr std::uint32_t k_PlayerC{ 0xE5A30003 };
    constexpr std::uint32_t k_WeaponA{ 0xAA010021 };
    constexpr std::uint32_t k_BipedA{ 0xEC010011 };
    constexpr std::uint32_t k_BipedB{ 0xEC020012 };

    constexpr std::string_view k_SpartanTag{ "test\\spartan" };
    constexpr std::string_view k_EliteTag{ "test\\elite" };
    constexpr std::string_view k_RifleTag{ "test\\rifle" };

    auto MakePlayer(std::uint32_t handle, std::uint32_t biped, Team team, const Vec3& aim, bool isAlive) -> Player
    {
        Player player{};
        player.Handle = handle;
        player.BipedHandle = biped;
        player.Team = team;
        if (isAlive)
        {
            player.Body = Body{};
            player.Body->Aim = aim;
        }
        return player;
    }

    // Player A (blue) at the origin with a rifle it carries, player B at x = -5, and a dead player C. A plane crosses the x axis in front of A
    // (its biped, at x = 2), another one crosses the y axis beside A (its rifle, at y = 2) and the one of B is where B stands (x = -5).
    class ViewRaysTests : public ::testing::Test, protected Chain
    {
    protected:
        auto SetUp() -> void override
        {
            auto& definitions{ m_Resolved.m_DefinitionsStore };
            definitions.m_Coll.Add("test\\plane_front", MakePlane("test\\plane_front", 0, 2.0f));
            definitions.m_Coll.Add("test\\plane_side", MakePlane("test\\plane_side", 1, 2.0f));
            definitions.m_Coll.Add("test\\plane_here", MakePlane("test\\plane_here", 0, 0.0f));
            Bipd bipd{};
            bipd.StandingCameraHeight = 0.62f;
            bipd.CrouchingCameraHeight = 0.42f;
            definitions.m_Bipd.Add(std::string{ k_SpartanTag }, bipd);
            definitions.m_Bipd.Add(std::string{ k_EliteTag }, bipd);
            definitions.Freeze();
            m_Resolved.m_WorldStore.LinkObjectColl(std::string{ k_SpartanTag }, "test\\plane_front");
            m_Resolved.m_WorldStore.LinkObjectColl(std::string{ k_RifleTag }, "test\\plane_side");
            m_Resolved.m_WorldStore.LinkObjectColl(std::string{ k_EliteTag }, "test\\plane_here");
            m_Resolved.m_WorldStore.Freeze();

            AliveObject weapon{ MakeObject(k_WeaponA, 0x3001, k_RifleTag, { 0.0f, 0.0f, 0.0f }) };
            weapon.ParentHandle = k_BipedA;
            m_Tables.m_ObjectStore.AddObject(k_BipedA, MakeObject(k_BipedA, 0x3002, k_SpartanTag, { 0.0f, 0.0f, 0.0f }));
            m_Tables.m_ObjectStore.AddObject(k_WeaponA, weapon);
            m_Tables.m_ObjectStore.AddObject(k_BipedB, MakeObject(k_BipedB, 0x3003, k_EliteTag, { -5.0f, 0.0f, 0.0f }));
            m_Tables.m_ObjectStore.Publish();

            this->PublishEntities(Team::Red);
            this->PublishPlayers(true);

            ObjectNodes nodes{};
            nodes.Nodes = { ObjectNode{ .Handle = k_WeaponA, .ParentHandle = k_BipedA },
                ObjectNode{ .Handle = k_BipedA, .FirstChild = 0, .ChildCount = 1 }, ObjectNode{ .Handle = k_BipedB } };
            nodes.Children = { k_WeaponA };
            m_Environment.m_ObjectGraphStore.Publish(std::move(nodes));
        }

        // Entities of the three objects: the team of the biped of B is the one the test gives.
        auto PublishEntities(Team teamOfB) -> void
        {
            Entities entities{};
            entities.Rows = { MakeEntity(k_WeaponA, Function::Weapon, Team::Neutral), MakeEntity(k_BipedA, Function::Combatant, Team::Blue),
                MakeEntity(k_BipedB, Function::Combatant, teamOfB) };
            entities.Rows[0].ParentHandle = k_BipedA;
            m_Environment.m_EntitiesStore.Publish(std::move(entities));
        }

        // Players A and B alive, with the aim of A to the given direction, and a dead C.
        auto PublishPlayers(bool isCAlive, const Vec3& aimOfA = { 1.0f, 0.0f, 0.0f }) -> void
        {
            Players players{};
            players.Rows = { MakePlayer(k_PlayerA, k_BipedA, Team::Blue, aimOfA, true), MakePlayer(k_PlayerB, k_BipedB, Team::Red, { 1.0f, 0.0f, 0.0f }, true),
                MakePlayer(k_PlayerC, k_NoHandle, Team::Red, {}, isCAlive) };
            for (std::size_t row{ 0 }; row < players.Rows.size(); ++row) players.Index[players.Rows[row].Handle] = row;
            m_Environment.m_PlayersStore.Publish(std::move(players));
        }

        auto Compute(const AgentSet& agents) -> ViewTable
        {
            m_Egocentric.m_ViewsService.Update(agents);
            const auto table{ m_Egocentric.m_ViewsStore.Acquire() };
            return table ? *table : ViewTable{};
        }

        auto Prepares() -> std::size_t
        {
            return m_Egocentric.m_RaycastSceneStore.Scene().Prepares;
        }
    };
}

TEST_F(ViewRaysTests, ExclusionNeverHitsOwnBipedOrCarried)
{
    const ViewTable table{ this->Compute(AgentSet::All()) };
    const View& a{ table[0] };
    const View& b{ table[1] };
    ASSERT_TRUE(a.Head.IsComputed);
    ASSERT_TRUE(b.Head.IsComputed);

    // Rays of the pattern: 0 right, 1 left (+y), 4 forward (+x), 5 back (-x). A does not see its own biped plane in front of it, nor the one of
    // the rifle it carries on its left: both are excluded. Behind it the plane of B is hit.
    EXPECT_EQ(a.Rays[4].Kind, HitKind::None);
    EXPECT_EQ(a.Rays[4].Distance, 20.0f);
    EXPECT_EQ(a.Rays[1].Kind, HitKind::None);
    EXPECT_EQ(a.Rays[0].Kind, HitKind::None);
    EXPECT_EQ(a.Rays[5].Kind, HitKind::Dynamic);
    EXPECT_NEAR(a.Rays[5].Distance, 5.0f, 1.0e-3f);
    EXPECT_EQ(a.Rays[5].Function, Function::Combatant);
    EXPECT_EQ(a.Rays[5].Relation, ViewRelation::Enemy);
    EXPECT_EQ(a.AimHit.Kind, HitKind::None);

    // The same planes do hit the rays of B: the plane of A at 7 m in front, and the rifle of A beside it, a weapon with no team.
    EXPECT_EQ(b.Rays[4].Kind, HitKind::Dynamic);
    EXPECT_NEAR(b.Rays[4].Distance, 7.0f, 1.0e-3f);
    EXPECT_EQ(b.Rays[4].Function, Function::Combatant);
    EXPECT_EQ(b.Rays[4].Relation, ViewRelation::Enemy);
    EXPECT_EQ(b.Rays[1].Kind, HitKind::Dynamic);
    EXPECT_NEAR(b.Rays[1].Distance, 2.0f, 1.0e-3f);
    EXPECT_EQ(b.Rays[1].Function, Function::Weapon);
    EXPECT_EQ(b.Rays[1].Relation, ViewRelation::Neutral);

    // The two rays that the pattern leaves free stay zero.
    EXPECT_EQ(a.Rays[14].Kind, HitKind::None);
    EXPECT_EQ(a.Rays[15].Distance, 0.0f);
}

TEST_F(ViewRaysTests, AimHitRelationIsEnemyAllyNeutral)
{
    // A aims back, at the plane of B: 5 m from the eye, which is above the origin.
    this->PublishPlayers(true, { -1.0f, 0.0f, 0.0f });

    const std::array<std::pair<Team, ViewRelation>, 3> cases{ {
        { Team::Red, ViewRelation::Enemy }, { Team::Blue, ViewRelation::Ally }, { Team::Neutral, ViewRelation::Neutral } } };
    for (const auto& [team, relation] : cases)
    {
        this->PublishEntities(team);
        const View& view{ this->Compute(AgentSet::All())[0] };
        EXPECT_NEAR(view.Self.Eye.Z, 0.62f, 1.0e-5f);
        EXPECT_EQ(view.AimHit.Kind, HitKind::Dynamic);
        EXPECT_NEAR(view.AimHit.Distance, 5.0f, 1.0e-3f);
        EXPECT_EQ(view.AimHit.ObjectHandle, k_BipedB);
        EXPECT_EQ(view.AimHit.Function, Function::Combatant);
        EXPECT_EQ(view.AimHit.PlayerRow, 1);
        EXPECT_EQ(view.AimHit.Relation, relation) << static_cast<int>(team);
    }

    // A miss has no object, no player and no relation, and its distance is the range of the aim (50 without a weapon).
    this->PublishPlayers(true, { 0.0f, 0.0f, 1.0f });
    const View& up{ this->Compute(AgentSet::All())[0] };
    EXPECT_EQ(up.AimHit.Kind, HitKind::None);
    EXPECT_EQ(up.AimHit.Distance, 50.0f);
    EXPECT_EQ(up.AimHit.ObjectHandle, k_NoHandle);
    EXPECT_EQ(up.AimHit.PlayerRow, 0xFF);
    EXPECT_EQ(up.AimHit.Relation, ViewRelation::None);
}

TEST_F(ViewRaysTests, SceneIsGatheredOncePerUpdate)
{
    this->PublishPlayers(false);
    const std::size_t before{ this->Prepares() };

    // Two agents with a body in one Update: one gather.
    this->Compute(AgentSet::All());
    EXPECT_EQ(this->Prepares(), before + 1);

    // Another Update gathers again: it is once per Update, not once per table.
    this->Compute(AgentSet::All());
    EXPECT_EQ(this->Prepares(), before + 2);

    // With no agent, or only a dead one, nothing is gathered.
    this->Compute(AgentSet{});
    AgentSet dead{};
    dead.Add(k_PlayerC);
    this->Compute(dead);
    EXPECT_EQ(this->Prepares(), before + 2);
}

TEST_F(ViewRaysTests, DeadAgentHasNoRays)
{
    this->PublishPlayers(false);
    AgentSet dead{};
    dead.Add(k_PlayerC);
    const ViewTable table{ this->Compute(dead) };

    const View& view{ table[2] };
    EXPECT_TRUE(view.Head.IsComputed);
    EXPECT_FALSE(view.Head.HasBody);
    EXPECT_EQ(view.AimHit.Kind, HitKind::None);
    EXPECT_EQ(view.AimHit.Distance, 0.0f);
    for (const auto& ray : view.Rays)
    {
        EXPECT_EQ(ray.Kind, HitKind::None);
        EXPECT_EQ(ray.Distance, 0.0f);
        EXPECT_EQ(ray.Relation, ViewRelation::None);
    }
    EXPECT_FALSE(table[0].Head.IsComputed);
}

// The watched player is the one with the observed handle, else the primary local player, and nobody when neither has a computed View.
TEST(ViewObservedTests, FindsTheObservedPlayerThenTheLocalOne)
{
    ViewTable table{};
    table[0].Head = { .PlayerHandle = k_PlayerA, .PlayerRow = 0, .LocalIndex = 0xFF, .IsComputed = true };
    table[1].Head = { .PlayerHandle = k_PlayerB, .PlayerRow = 1, .LocalIndex = 0, .IsComputed = true };
    table[2].Head = { .PlayerHandle = k_PlayerC, .PlayerRow = 2, .LocalIndex = 0, .IsComputed = false };

    EXPECT_EQ(FindObservedView(table, k_PlayerA), &table[0]);
    EXPECT_EQ(FindObservedView(table, k_NoHandle), &table[1]);
    EXPECT_EQ(FindObservedView(table, 0xDEAD0001), &table[1]);
    EXPECT_EQ(FindObservedView(table, k_PlayerC), &table[1]);

    table[1].Head.IsComputed = false;
    EXPECT_EQ(FindObservedView(table, k_NoHandle), nullptr);
    EXPECT_EQ(FindObservedView(table, k_PlayerA), &table[0]);
}

// The raycasts the dev tools draw come out of the View: the aim ray from the eye, the pattern from the ray origin in the frame of the aim.
TEST_F(ViewRaysTests, RaycastsOfRebuildsTheRaysOfTheView)
{
    AgentSet agents{};
    agents.Add(k_PlayerA);
    agents.Add(k_PlayerC);
    const ViewTable table{ this->Compute(agents) };
    const View& view{ table[0] };
    ASSERT_TRUE(view.Head.IsComputed);
    EXPECT_EQ(view.Head.BipedHandle, k_BipedA);
    EXPECT_EQ(view.Head.LocalIndex, 0xFF);

    const auto rebuilt{ RaycastsOf(view) };
    EXPECT_EQ(rebuilt.AimHit.Origin.Z, view.Self.Eye.Z);
    EXPECT_NEAR(rebuilt.AimHit.Direction.X, 1.0f, 1.0e-5f);
    EXPECT_EQ(rebuilt.AimHit.Hit, view.AimHit.Kind != HitKind::None);
    EXPECT_EQ(rebuilt.AimHit.MaxDistance, view.AimHit.MaxDistance);

    ASSERT_EQ(rebuilt.PerceptionHits.size(), 14u);
    for (std::size_t index{ 0 }; index < rebuilt.PerceptionHits.size(); ++index)
    {
        const auto& hit{ rebuilt.PerceptionHits[index] };
        const auto& ray{ view.Rays[index] };
        const float length{ std::sqrt(hit.Direction.X * hit.Direction.X + hit.Direction.Y * hit.Direction.Y + hit.Direction.Z * hit.Direction.Z) };
        EXPECT_NEAR(length, 1.0f, 1.0e-5f) << index;
        EXPECT_EQ(hit.MaxDistance, 20.0f) << index;
        EXPECT_EQ(hit.Hit, ray.Kind != HitKind::None) << index;
        if (hit.Hit) EXPECT_NEAR(hit.Origin.X + hit.Direction.X * ray.Distance, hit.Point.X, 1.0e-4f) << index;
    }
    EXPECT_NEAR(rebuilt.PerceptionHits[4].Direction.X, 1.0f, 1.0e-5f);

    // A dead agent has no rays, and its aim was not cast.
    const auto dead{ RaycastsOf(table[2]) };
    EXPECT_TRUE(dead.PerceptionHits.empty());
    EXPECT_FALSE(dead.AimHit.Hit);
}
