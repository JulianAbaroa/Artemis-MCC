#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.Type;
import Perception.Details.Type;
import Perception.Entities.Type;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;
import Views.Raycast.Type;
import Views.View.System;
import Views.View.Type;
import Tests.Support.FilmPass;
import Tests.Support.Pipeline;
import Tests.Support.ViewScene;
import std;

// Who sees whom, what each agent knows of the other players and where the flags are (F4.3, D308, D310, D113), over made-up planes the
// lines of sight cross, and over two recorded matches for the invariants of the design (I1, I2, I3, I5 and I6).

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::FlagState;
    using Common::Semantic::Type::Function;
    using Common::Team::Type::Team;
    using Blam::MapModel::Definitions::Type::Bipd::Bipd;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::StandDetail;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Perception::ObjectGraph::Type::ObjectNode;
    using Perception::ObjectGraph::Type::ObjectNodes;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::ViewScene::MakeEntity;
    using Tests::Support::ViewScene::MakeObject;
    using Tests::Support::ViewScene::MakePlane;
    using Views::View::System::AimDirectionOf;
    using Views::View::Type::AgentSet;
    using Views::View::Type::View;
    using Views::View::Type::ViewChannel;
    using Views::View::Type::ViewFlag;
    using Views::View::Type::ViewMode;
    using Views::View::Type::ViewOther;
    using Views::View::Type::ViewRelation;
    using Views::View::Type::ViewTable;

    using Common::Engine::Type::k_NoHandle;
    using Views::Raycast::Type::Sight::k_SightHalfAngle;
    using Views::Raycast::Type::Sight::k_SightRange;

    constexpr std::size_t k_Slots{ 4 };
    constexpr std::array<std::uint32_t, k_Slots> k_PlayerHandles{ 0xE5A10001, 0xE5A20002, 0xE5A30003, 0xE5A40004 };
    constexpr std::array<std::uint32_t, k_Slots> k_BipedHandles{ 0xEC010011, 0xEC020012, 0xEC030013, 0xEC040014 };
    constexpr std::array<std::uint32_t, k_Slots> k_WeaponHandles{ 0xAA010021, 0xAA020022, 0xAA030023, 0xAA040024 };
    constexpr std::uint32_t k_WallHandle{ 0xBB010031 };
    constexpr std::uint32_t k_FlagBlue{ 0xAA0100F1 };
    constexpr std::uint32_t k_FlagRed{ 0xAA0100F2 };
    constexpr std::uint32_t k_StandBlue{ 0xB0000001 };
    constexpr std::uint32_t k_StandRed{ 0xB0000002 };
    constexpr std::uint8_t k_RifleSubtype{ 7 };
    constexpr float k_EyeHeight{ 0.62f };

    constexpr std::string_view k_SpartanTag{ "test\\spartan" };
    constexpr std::string_view k_SolidTag{ "test\\spartan_solid" };
    constexpr std::string_view k_RifleTag{ "test\\rifle" };
    constexpr std::string_view k_WallTag{ "test\\wall" };

    // A player of a made-up scene. A solid biped has a plane 2 m in front of it that its own lines of sight must ignore.
    struct Actor
    {
        bool IsPresent{};
        bool IsAlive{ true };
        bool IsSolid{};
        bool HasRifle{};
        bool IsCrouching{};
        Team Allegiance{ Team::Neutral };
        Vec3 Position{};
        Vec3 Aim{ 1.0f, 0.0f, 0.0f };
        Vec3 Velocity{};
        float Vitality{ 1.0f };
        float Shield{ 0.5f };
        std::uint32_t Carried{ k_NoHandle };
    };

    auto IsZeroVec(const Vec3& value) -> bool
    {
        return value.X == 0.0f && value.Y == 0.0f && value.Z == 0.0f;
    }

    auto IsSamePoint(const Vec3& a, const Vec3& b) -> bool
    {
        return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
    }

    // return: Whether the row has no channel and every field but Relation is zero (I5).
    auto IsZeroButRelation(const ViewOther& row) -> bool
    {
        return row.Channels == 0 && !row.IsAlive && IsZeroVec(row.Position) && IsZeroVec(row.Velocity) && row.FacingYaw == 0.0f &&
            row.Vitality == 0.0f && row.Shield == 0.0f && row.WeaponSubtype == 0 && !row.IsCrouching && !row.CarriesFlag && row.Elevation == 0;
    }

    auto IsZeroFlag(const ViewFlag& flag) -> bool
    {
        return flag.State == FlagState::Home && IsZeroVec(flag.Position) && flag.CarrierRow == 0 && flag.ResetTicks == 0 && !flag.IsResetRunning &&
            flag.AlliesInZone == 0 && IsZeroVec(flag.StandPosition);
    }

    constexpr auto k_Hud{ static_cast<std::uint8_t>(ViewChannel::Hud) };
    constexpr auto k_Seen{ static_cast<std::uint8_t>(ViewChannel::Seen) };

    // Up to four players with a biped each, a wall that the lines of sight cross, and the flags. Heads are at the feet plus 0.62 (the standing eye).
    class ViewStage : public ::testing::Test, protected Chain
    {
    protected:
        std::array<Actor, k_Slots> m_Actors{};
        std::optional<float> m_WallX{};
        std::vector<Entity> m_FlagEntities{};

        auto SetUp() -> void override
        {
            auto& definitions{ m_Resolved.m_DefinitionsStore };
            definitions.m_Coll.Add("test\\plane_front", MakePlane("test\\plane_front", 0, 2.0f));
            definitions.m_Coll.Add("test\\plane_near", MakePlane("test\\plane_near", 0, -1.0f));
            definitions.m_Coll.Add("test\\plane_wall", MakePlane("test\\plane_wall", 0, 0.0f));
            Bipd bipd{};
            bipd.StandingCameraHeight = k_EyeHeight;
            bipd.CrouchingCameraHeight = 0.42f;
            definitions.m_Bipd.Add(std::string{ k_SpartanTag }, bipd);
            definitions.m_Bipd.Add(std::string{ k_SolidTag }, bipd);
            definitions.Freeze();
            m_Resolved.m_WorldStore.LinkObjectColl(std::string{ k_SolidTag }, "test\\plane_front");
            m_Resolved.m_WorldStore.LinkObjectColl(std::string{ k_RifleTag }, "test\\plane_near");
            m_Resolved.m_WorldStore.LinkObjectColl(std::string{ k_WallTag }, "test\\plane_wall");
            m_Resolved.m_WorldStore.Freeze();
        }

        // Places the player of the slot. The slots must be used in order, from 0: the slot is the row of its player.
        auto Put(std::size_t slot, Team team, const Vec3& position, const Vec3& aim) -> Actor&
        {
            Actor& actor{ m_Actors[slot] };
            actor = Actor{ .IsPresent = true, .Allegiance = team, .Position = position, .Aim = aim };
            return actor;
        }

        // Publishes the objects, entities, graph and players of the scene as it stands.
        auto Publish() -> void
        {
            Entities entities{};
            Players players{};
            std::vector<ObjectNode> nodeRows{};
            std::vector<std::uint32_t> children{};

            for (std::size_t slot{ 0 }; slot < k_Slots; ++slot)
            {
                const Actor& actor{ m_Actors[slot] };
                m_Tables.m_ObjectStore.RemoveObject(k_BipedHandles[slot]);
                m_Tables.m_ObjectStore.RemoveObject(k_WeaponHandles[slot]);
                if (!actor.IsPresent) continue;

                Player player{};
                player.Handle = k_PlayerHandles[slot];
                player.Team = actor.Allegiance;
                if (actor.IsAlive)
                {
                    player.BipedHandle = k_BipedHandles[slot];
                    Body body{};
                    body.Aim = actor.Aim;
                    body.Shield = actor.Shield;
                    body.IsCrouching = actor.IsCrouching;
                    body.Loadout.Objective = actor.Carried;
                    if (actor.HasRifle)
                    {
                        body.Loadout.PrimaryWeapon = k_WeaponHandles[slot];
                        body.Loadout.EquippedWeapon = k_WeaponHandles[slot];
                    }
                    player.Body = body;

                    m_Tables.m_ObjectStore.AddObject(k_BipedHandles[slot],
                        MakeObject(k_BipedHandles[slot], actor.IsSolid ? 0x3004 : 0x3002, actor.IsSolid ? k_SolidTag : k_SpartanTag, actor.Position));
                    Entity biped{ MakeEntity(k_BipedHandles[slot], Function::Combatant, actor.Allegiance) };
                    biped.WorldOrigin = actor.Position;
                    biped.LinearVelocity = actor.Velocity;
                    biped.Vitality = actor.Vitality;
                    entities.Rows.push_back(biped);

                    ObjectNode node{ .Handle = k_BipedHandles[slot] };
                    if (actor.HasRifle)
                    {
                        this->AddRifle(slot, actor);
                        Entity weapon{ MakeEntity(k_WeaponHandles[slot], Function::Weapon, Team::Neutral) };
                        weapon.ParentHandle = k_BipedHandles[slot];
                        weapon.Subtype = k_RifleSubtype;
                        entities.Rows.push_back(weapon);
                        nodeRows.push_back(ObjectNode{ .Handle = k_WeaponHandles[slot], .ParentHandle = k_BipedHandles[slot] });
                        node.FirstChild = static_cast<std::uint32_t>(children.size());
                        node.ChildCount = 1;
                        children.push_back(k_WeaponHandles[slot]);
                    }
                    nodeRows.push_back(node);
                }
                players.Rows.push_back(player);
            }

            m_Tables.m_ObjectStore.RemoveObject(k_WallHandle);
            if (m_WallX) m_Tables.m_ObjectStore.AddObject(k_WallHandle, MakeObject(k_WallHandle, 0x3005, k_WallTag, { *m_WallX, 0.0f, 0.0f }));
            m_Tables.m_ObjectStore.Publish();

            for (const Entity& flag : m_FlagEntities) entities.Rows.push_back(flag);
            std::ranges::sort(entities.Rows, {}, &Entity::Handle);
            std::ranges::sort(nodeRows, {}, &ObjectNode::Handle);
            ObjectNodes nodes{};
            nodes.Nodes = std::move(nodeRows);
            nodes.Children = std::move(children);
            for (std::size_t row{ 0 }; row < players.Rows.size(); ++row) players.Index[players.Rows[row].Handle] = row;

            m_Environment.m_EntitiesStore.Publish(std::move(entities));
            m_Environment.m_PlayersStore.Publish(std::move(players));
            m_Environment.m_ObjectGraphStore.Publish(std::move(nodes));
        }

        auto PublishObjectives(const std::vector<ObjectiveDetail>& flags, const std::vector<StandDetail>& stands) -> void
        {
            ObjectiveDetails details{};
            for (const ObjectiveDetail& flag : flags) details.Flags[flag.Handle] = flag;
            for (const StandDetail& stand : stands) details.Stands[stand.Handle] = stand;
            m_Environment.m_ObjectiveDetailsStore.Publish(std::move(details));
        }

        auto Compute(const AgentSet& agents) -> ViewTable
        {
            m_Egocentric.m_ViewsService.Update(agents);
            const auto table{ m_Egocentric.m_ViewsStore.Acquire() };
            return table ? *table : ViewTable{};
        }

        auto Candidates() -> std::size_t
        {
            return m_Egocentric.m_RaycastSceneStore.Scene().SightCandidates;
        }

        auto Rays() -> std::size_t
        {
            return m_Egocentric.m_RaycastSceneStore.Scene().SightRays;
        }

    private:
        // The rifle is held by the biped: it is an object of its own that the exclusion of the biped reaches through the graph.
        auto AddRifle(std::size_t slot, const Actor& actor) -> void
        {
            auto rifle{ MakeObject(k_WeaponHandles[slot], 0x3001, k_RifleTag, actor.Position) };
            rifle.ParentHandle = k_BipedHandles[slot];
            m_Tables.m_ObjectStore.AddObject(k_WeaponHandles[slot], rifle);
        }
    };

    class ViewSightTests : public ViewStage
    {
    };

    class ViewOthersTests : public ViewStage
    {
    };

    class ViewFlagsTests : public ViewStage
    {
    };

    auto At(double degrees, double distance) -> Vec3
    {
        const double radians{ degrees * 3.14159265358979323846 / 180.0 };
        return Vec3{ static_cast<float>(distance * std::cos(radians)), static_cast<float>(distance * std::sin(radians)), 0.0f };
    }
}

// A wall between two players that look at each other hides them from each other, and when the wall goes they see each other.
TEST_F(ViewSightTests, BehindWallIsNotSeen)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { 10.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f });
    m_WallX = 5.0f;
    this->Publish();

    ViewTable views{ this->Compute(AgentSet::All()) };
    EXPECT_EQ(this->Candidates(), 1u);
    EXPECT_EQ(this->Rays(), 1u);
    EXPECT_EQ(views[0].Others[1].Relation, ViewRelation::Enemy);
    EXPECT_TRUE(IsZeroButRelation(views[0].Others[1]));
    EXPECT_EQ(views[1].Others[0].Relation, ViewRelation::Enemy);
    EXPECT_TRUE(IsZeroButRelation(views[1].Others[0]));

    m_WallX.reset();
    this->Publish();
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(views[0].Others[1].Channels, k_Seen);
    EXPECT_TRUE(views[0].Others[1].IsAlive);
    EXPECT_TRUE(IsSamePoint(views[0].Others[1].Position, { 10.0f, 0.0f, 0.0f }));
    EXPECT_EQ(views[1].Others[0].Channels, k_Seen);
}

// Only a player inside the cone is a candidate, and one past the range is not either.
TEST_F(ViewSightTests, OutsideConeIsNotSeen)
{
    // B looks away from A, so only the cone of A counts: 46 degrees is out of the 45 of the half angle and 44 is in.
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, At(46.0, 10.0), { 1.0f, 0.0f, 0.0f });
    this->Publish();

    ViewTable views{ this->Compute(AgentSet::All()) };
    EXPECT_EQ(this->Candidates(), 0u);
    EXPECT_EQ(this->Rays(), 0u);
    EXPECT_TRUE(IsZeroButRelation(views[0].Others[1]));

    this->Put(1, Team::Red, At(44.0, 10.0), { 1.0f, 0.0f, 0.0f });
    this->Publish();
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(this->Candidates(), 1u);
    EXPECT_EQ(this->Rays(), 1u);
    EXPECT_EQ(views[0].Others[1].Channels, k_Seen);
    EXPECT_TRUE(IsZeroButRelation(views[1].Others[0]));

    // In the cone but past k_SightRange.
    this->Put(1, Team::Red, { k_SightRange + 10.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Publish();
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(this->Candidates(), 0u);
    EXPECT_TRUE(IsZeroButRelation(views[0].Others[1]));
}

// Three living players in a line: the three pairs are candidates, one ray each, even where both players of a pair look at each other. The dead
// player is in no pair, and the counters start again at every Update.
TEST_F(ViewSightTests, OneRayPerUnorderedPair)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { 10.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f });
    this->Put(2, Team::Red, { 20.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f });
    this->Put(3, Team::Blue, { 5.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }).IsAlive = false;
    this->Publish();

    this->Compute(AgentSet::All());
    EXPECT_EQ(this->Candidates(), 3u);
    EXPECT_EQ(this->Rays(), 3u);

    this->Compute(AgentSet::All());
    EXPECT_EQ(this->Rays(), 3u);

    this->Compute(AgentSet{});
    EXPECT_EQ(this->Candidates(), 0u);
    EXPECT_EQ(this->Rays(), 0u);
}

// The line between two heads is one: it hides both or none. The cone is each one's own, so one can see the other that does not see it back.
TEST_F(ViewSightTests, MutualConeIsSymmetric)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { 10.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f });
    this->Publish();

    ViewTable views{ this->Compute(AgentSet::All()) };
    EXPECT_EQ(this->Rays(), 1u);
    EXPECT_EQ(views[0].Others[1].Channels, k_Seen);
    EXPECT_EQ(views[1].Others[0].Channels, k_Seen);

    m_WallX = 5.0f;
    this->Publish();
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(views[0].Others[1].Channels, views[1].Others[0].Channels);
    EXPECT_EQ(views[0].Others[1].Channels, 0);

    // B turns away: A still sees B and B does not see A, with the same single ray.
    m_WallX.reset();
    m_Actors[1].Aim = { 1.0f, 0.0f, 0.0f };
    this->Publish();
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(this->Rays(), 1u);
    EXPECT_EQ(views[0].Others[1].Channels, k_Seen);
    EXPECT_TRUE(IsZeroButRelation(views[1].Others[0]));
}

// What the two players carry or ride, and their own bodies, never hide them from each other. A third body does.
TEST_F(ViewSightTests, OwnAndTargetGearNeverBlock)
{
    // A is solid: a plane 2 m in front of it. B holds a rifle with a plane 1 m in front of it, toward A. Both planes cross the line.
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }).IsSolid = true;
    this->Put(1, Team::Red, { 10.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }).HasRifle = true;
    this->Publish();

    ViewTable views{ this->Compute(AgentSet::All()) };
    EXPECT_EQ(this->Rays(), 1u);
    EXPECT_EQ(views[0].Others[1].Channels, k_Seen);
    EXPECT_EQ(views[1].Others[0].Channels, k_Seen);

    // A solid body of a third player, off to the side of both cones, across the same line.
    this->Put(2, Team::Red, { 3.5f, 5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).IsSolid = true;
    this->Publish();
    views = this->Compute(AgentSet::All());
    EXPECT_TRUE(IsZeroButRelation(views[0].Others[1]));
    EXPECT_TRUE(IsZeroButRelation(views[1].Others[0]));
}

// A dead player is not seen and does not see, even an ally, who stays on the Hud.
TEST_F(ViewSightTests, DeadIsNeverSeen)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { 10.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }).IsAlive = false;
    this->Put(2, Team::Blue, { 6.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }).IsAlive = false;
    this->Publish();

    const ViewTable views{ this->Compute(AgentSet::All()) };
    EXPECT_EQ(this->Candidates(), 0u);
    EXPECT_EQ(this->Rays(), 0u);
    EXPECT_TRUE(IsZeroButRelation(views[0].Others[1]));
    EXPECT_EQ(views[0].Others[2].Channels, k_Hud);
    EXPECT_FALSE(views[0].Others[2].IsAlive);
    EXPECT_TRUE(IsZeroVec(views[0].Others[2].Position));
    EXPECT_TRUE(IsZeroButRelation(views[1].Others[0]));
}

// Full mode sees every living player at once: behind the wall, behind the back, and with no ray at all. The dead are still not seen.
TEST_F(ViewSightTests, FullModeSeesEveryoneWithoutRays)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { -10.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(2, Team::Red, { 20.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }).IsAlive = false;
    m_WallX = -5.0f;
    this->Publish();

    const ViewTable views{ this->Compute(AgentSet{ .Mask = 0xFFFFFFFFu, .Mode = ViewMode::Full }) };
    EXPECT_EQ(this->Candidates(), 0u);
    EXPECT_EQ(this->Rays(), 0u);
    EXPECT_EQ(views[0].Head.Mode, ViewMode::Full);
    EXPECT_EQ(views[0].Others[0].Relation, ViewRelation::Self);
    EXPECT_EQ(views[0].Others[0].Channels, 0);
    EXPECT_EQ(views[0].Others[1].Channels, k_Seen);
    EXPECT_TRUE(views[0].Others[1].IsAlive);
    EXPECT_TRUE(IsSamePoint(views[0].Others[1].Position, { -10.0f, 0.0f, 0.0f }));
    EXPECT_TRUE(IsZeroButRelation(views[0].Others[2]));
    EXPECT_EQ(views[1].Others[0].Channels, k_Seen);
}

// An ally is on the Hud with its position even behind a wall and out of the cone, and nothing else of it. Seen adds the rest.
TEST_F(ViewOthersTests, AllyAlwaysHasHudPosition)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    Actor& ally{ this->Put(1, Team::Blue, { 0.0f, 30.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }) };
    ally.Velocity = { 1.0f, 2.0f, 3.0f };
    ally.Vitality = 0.8f;
    ally.HasRifle = true;
    ally.IsCrouching = true;
    ally.Carried = k_FlagRed;
    this->Publish();

    ViewTable views{ this->Compute(AgentSet::All()) };
    ViewOther row{ views[0].Others[1] };
    EXPECT_EQ(row.Relation, ViewRelation::Ally);
    EXPECT_EQ(row.Channels, k_Hud);
    EXPECT_TRUE(row.IsAlive);
    EXPECT_TRUE(row.CarriesFlag);
    EXPECT_TRUE(IsSamePoint(row.Position, { 0.0f, 30.0f, 0.0f }));
    EXPECT_TRUE(IsZeroVec(row.Velocity));
    EXPECT_EQ(row.FacingYaw, 0.0f);
    EXPECT_EQ(row.Vitality, 0.0f);
    EXPECT_EQ(row.Shield, 0.0f);
    EXPECT_EQ(row.WeaponSubtype, 0);
    EXPECT_FALSE(row.IsCrouching);

    // In front of A and in the clear: the Hud and Seen. A rifle plane 1 m before the ally does not hide it.
    m_Actors[1].Position = { 10.0f, 0.0f, 0.0f };
    this->Publish();
    views = this->Compute(AgentSet::All());
    row = views[0].Others[1];
    EXPECT_EQ(row.Channels, k_Hud | k_Seen);
    EXPECT_TRUE(IsSamePoint(row.Velocity, { 1.0f, 2.0f, 3.0f }));
    EXPECT_NEAR(row.FacingYaw, 1.5707963f, 1.0e-5f);
    EXPECT_EQ(row.Vitality, 0.8f);
    EXPECT_EQ(row.Shield, 0.5f);
    EXPECT_EQ(row.WeaponSubtype, k_RifleSubtype);
    EXPECT_TRUE(row.IsCrouching);
    EXPECT_TRUE(row.CarriesFlag);

    // A dead ally is still on the Hud, with no position.
    m_Actors[1].IsAlive = false;
    this->Publish();
    views = this->Compute(AgentSet::All());
    row = views[0].Others[1];
    EXPECT_EQ(row.Channels, k_Hud);
    EXPECT_FALSE(row.IsAlive);
    EXPECT_FALSE(row.CarriesFlag);
    EXPECT_TRUE(IsZeroVec(row.Position));
}

// A row with no channel is all zero but for its Relation: an enemy out of sight, a player without a team, the agent itself and the rows past the table.
TEST_F(ViewOthersTests, RowWithoutChannelIsZero)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    Actor& enemy{ this->Put(1, Team::Red, { 0.0f, 20.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }) };
    enemy.Velocity = { 4.0f, 5.0f, 6.0f };
    enemy.Carried = k_FlagBlue;
    this->Put(2, Team::Neutral, { 0.0f, -20.0f, 0.0f }, { 0.0f, -1.0f, 0.0f });
    this->Publish();

    const ViewTable views{ this->Compute(AgentSet::All()) };
    const View& view{ views[0] };
    EXPECT_EQ(view.Others[0].Relation, ViewRelation::Self);
    EXPECT_TRUE(IsZeroButRelation(view.Others[0]));
    EXPECT_EQ(view.Others[1].Relation, ViewRelation::Enemy);
    EXPECT_TRUE(IsZeroButRelation(view.Others[1]));
    EXPECT_EQ(view.Others[2].Relation, ViewRelation::None);
    EXPECT_TRUE(IsZeroButRelation(view.Others[2]));
    for (std::size_t row{ 3 }; row < view.Others.size(); ++row)
    {
        EXPECT_EQ(view.Others[row].Relation, ViewRelation::None) << row;
        EXPECT_TRUE(IsZeroButRelation(view.Others[row])) << row;
    }
}

// The enemy that carries a flag is not on the Others unless it is seen: where it is, is in the Flags.
TEST_F(ViewOthersTests, EnemyCarrierPositionOnlyInFlags)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { 0.0f, 20.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).Carried = k_FlagBlue;
    Entity flag{ MakeEntity(k_FlagBlue, Function::Weapon, Team::Blue) };
    flag.WorldOrigin = { 99.0f, 99.0f, 99.0f };
    m_FlagEntities.push_back(flag);
    this->PublishObjectives({ ObjectiveDetail{ .Handle = k_FlagBlue, .BaseTeam = Team::Blue, .OwnerTeam = Team::Blue, .State = FlagState::Carried,
        .CarrierPlayer = k_PlayerHandles[1], .CarrierBiped = k_BipedHandles[1], .HomeStand = k_StandBlue } },
        { StandDetail{ .Handle = k_StandBlue, .BaseTeam = Team::Blue, .OwnerTeam = Team::Blue, .WorldOrigin = { -50.0f, 0.0f, 0.0f } } });
    this->Publish();

    ViewTable views{ this->Compute(AgentSet::All()) };
    EXPECT_TRUE(IsZeroButRelation(views[0].Others[1]));
    EXPECT_FALSE(views[0].Others[1].CarriesFlag);
    EXPECT_EQ(views[0].Flags[0].State, FlagState::Carried);
    EXPECT_TRUE(IsSamePoint(views[0].Flags[0].Position, { 0.0f, 20.0f, 0.0f }));
    EXPECT_EQ(views[0].Flags[0].CarrierRow, 1);

    // Seen, the carrier is a row like any other.
    m_Actors[1].Position = { 10.0f, 0.0f, 0.0f };
    m_Actors[1].Aim = { -1.0f, 0.0f, 0.0f };
    this->Publish();
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(views[0].Others[1].Channels, k_Seen);
    EXPECT_TRUE(views[0].Others[1].CarriesFlag);
    EXPECT_TRUE(IsSamePoint(views[0].Others[1].Position, { 10.0f, 0.0f, 0.0f }));
}

// [0] is the flag whose side the team plays in this round and [1] the rival one, whatever their base teams are.
TEST_F(ViewFlagsTests, OwnAndEnemyBySideOfTheRound)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { 0.0f, 20.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    this->Put(2, Team::Neutral, { 0.0f, -20.0f, 0.0f }, { 0.0f, -1.0f, 0.0f });
    Entity redBased{ MakeEntity(k_FlagRed, Function::Weapon, Team::Red) };
    redBased.WorldOrigin = { 1.0f, 2.0f, 3.0f };
    Entity blueBased{ MakeEntity(k_FlagBlue, Function::Weapon, Team::Blue) };
    blueBased.WorldOrigin = { 4.0f, 5.0f, 6.0f };
    m_FlagEntities = { redBased, blueBased };

    // The sides are swapped in this round: the red based flag is the one Blue plays, at home, and the blue based one is the one Red plays, dropped.
    this->PublishObjectives({
        ObjectiveDetail{ .Handle = k_FlagRed, .BaseTeam = Team::Red, .OwnerTeam = Team::Blue, .State = FlagState::Home, .HomeStand = k_StandRed, .AlliesInZone = 2 },
        ObjectiveDetail{ .Handle = k_FlagBlue, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red, .State = FlagState::Dropped, .HomeStand = k_StandBlue,
            .ResetTicks = 120, .IsResetRunning = true, .AlliesInZone = 1 } },
        { StandDetail{ .Handle = k_StandRed, .BaseTeam = Team::Red, .OwnerTeam = Team::Blue, .WorldOrigin = { 10.0f, 20.0f, 30.0f } },
            StandDetail{ .Handle = k_StandBlue, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red, .WorldOrigin = { 40.0f, 50.0f, 60.0f } } });
    this->Publish();

    ViewTable views{ this->Compute(AgentSet::All()) };
    for (const auto& [row, own, rival] : { std::tuple{ 0, 1.0f, 4.0f }, std::tuple{ 1, 4.0f, 1.0f } })
    {
        EXPECT_EQ(views[row].Flags[0].Position.X, own) << row;
        EXPECT_EQ(views[row].Flags[1].Position.X, rival) << row;
    }
    EXPECT_EQ(views[0].Flags[0].State, FlagState::Home);
    EXPECT_EQ(views[0].Flags[0].CarrierRow, 0xFF);
    EXPECT_EQ(views[0].Flags[0].AlliesInZone, 2);
    EXPECT_TRUE(IsSamePoint(views[0].Flags[0].StandPosition, { 10.0f, 20.0f, 30.0f }));
    EXPECT_EQ(views[0].Flags[1].State, FlagState::Dropped);
    EXPECT_EQ(views[0].Flags[1].ResetTicks, 120u);
    EXPECT_TRUE(views[0].Flags[1].IsResetRunning);
    EXPECT_TRUE(IsSamePoint(views[0].Flags[1].StandPosition, { 40.0f, 50.0f, 60.0f }));
    EXPECT_EQ(views[1].Flags[0].State, FlagState::Dropped);
    EXPECT_EQ(views[1].Flags[1].State, FlagState::Home);

    // Without a team there is no side, and without flags (not a capture game) both rows are zero.
    EXPECT_TRUE(IsZeroFlag(views[2].Flags[0]));
    EXPECT_TRUE(IsZeroFlag(views[2].Flags[1]));
    this->PublishObjectives({}, {});
    views = this->Compute(AgentSet::All());
    EXPECT_TRUE(IsZeroFlag(views[0].Flags[0]));
    EXPECT_TRUE(IsZeroFlag(views[0].Flags[1]));
}

// A flag in hands is where its carrier is, and its row is the carrier's row, whether the carrier is a player of the table or only a biped.
TEST_F(ViewFlagsTests, CarriedFlagFollowsCarrier)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { 10.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f });
    Entity flag{ MakeEntity(k_FlagBlue, Function::Weapon, Team::Blue) };
    flag.WorldOrigin = { 99.0f, 99.0f, 99.0f };
    m_FlagEntities = { flag };
    const auto publish{ [this](FlagState state, std::uint32_t player, std::uint32_t biped) {
        this->PublishObjectives({ ObjectiveDetail{ .Handle = k_FlagBlue, .BaseTeam = Team::Blue, .OwnerTeam = Team::Blue, .State = state,
            .CarrierPlayer = player, .CarrierBiped = biped } }, {});
    } };
    publish(FlagState::Carried, k_PlayerHandles[1], k_BipedHandles[1]);
    this->Publish();

    ViewTable views{ this->Compute(AgentSet::All()) };
    EXPECT_TRUE(IsSamePoint(views[0].Flags[0].Position, { 10.0f, 0.0f, 0.0f }));
    EXPECT_EQ(views[0].Flags[0].CarrierRow, 1);
    EXPECT_EQ(views[1].Flags[1].CarrierRow, 1);

    m_Actors[1].Position = { 20.0f, 0.0f, 0.0f };
    this->Publish();
    views = this->Compute(AgentSet::All());
    EXPECT_TRUE(IsSamePoint(views[0].Flags[0].Position, { 20.0f, 0.0f, 0.0f }));

    // A carrier that is not in the table of players still gives its biped, and no row.
    publish(FlagState::Carried, k_NoHandle, k_BipedHandles[1]);
    views = this->Compute(AgentSet::All());
    EXPECT_TRUE(IsSamePoint(views[0].Flags[0].Position, { 20.0f, 0.0f, 0.0f }));
    EXPECT_EQ(views[0].Flags[0].CarrierRow, 1);

    // Dropped, the flag is where its own object is and nobody carries it.
    publish(FlagState::Dropped, k_NoHandle, k_NoHandle);
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(views[0].Flags[0].State, FlagState::Dropped);
    EXPECT_TRUE(IsSamePoint(views[0].Flags[0].Position, { 99.0f, 99.0f, 99.0f }));
    EXPECT_EQ(views[0].Flags[0].CarrierRow, 0xFF);
}

// Every field of a flag row is the one of ObjectiveDetails (or of its entity), for each agent.
TEST_F(ViewFlagsTests, MatchesObjectiveDetails)
{
    this->Put(0, Team::Blue, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
    this->Put(1, Team::Red, { 0.0f, 20.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    this->Put(2, Team::Blue, { 0.0f, -20.0f, 0.0f }, { 0.0f, -1.0f, 0.0f });
    Entity red{ MakeEntity(k_FlagRed, Function::Weapon, Team::Red) };
    red.WorldOrigin = { 7.0f, 8.0f, 9.0f };
    Entity blue{ MakeEntity(k_FlagBlue, Function::Weapon, Team::Blue) };
    m_FlagEntities = { red, blue };
    const std::vector<ObjectiveDetail> flags{
        ObjectiveDetail{ .Handle = k_FlagRed, .BaseTeam = Team::Red, .OwnerTeam = Team::Red, .State = FlagState::Dropped, .HomeStand = k_StandRed,
            .ResetTicks = 77, .IsResetRunning = true, .AlliesInZone = 3 },
        ObjectiveDetail{ .Handle = k_FlagBlue, .BaseTeam = Team::Blue, .OwnerTeam = Team::Blue, .State = FlagState::Carried, .CarrierPlayer = k_PlayerHandles[1],
            .CarrierBiped = k_BipedHandles[1], .HomeStand = k_StandBlue, .AlliesInZone = 1 } };
    this->PublishObjectives(flags, { StandDetail{ .Handle = k_StandRed, .BaseTeam = Team::Red, .OwnerTeam = Team::Red, .WorldOrigin = { 1.5f, 2.5f, 3.5f } },
        StandDetail{ .Handle = k_StandBlue, .BaseTeam = Team::Blue, .OwnerTeam = Team::Blue, .WorldOrigin = { -1.5f, -2.5f, -3.5f } } });
    this->Publish();

    const ViewTable views{ this->Compute(AgentSet::All()) };
    for (std::size_t row{ 0 }; row < 3; ++row)
    {
        const Team team{ m_Actors[row].Allegiance };
        const ObjectiveDetail& own{ flags[team == Team::Red ? 0 : 1] };
        const ObjectiveDetail& rival{ flags[team == Team::Red ? 1 : 0] };
        const std::array<const ObjectiveDetail*, 2> expected{ &own, &rival };
        for (std::size_t index{ 0 }; index < 2; ++index)
        {
            const ViewFlag& flag{ views[row].Flags[index] };
            const ObjectiveDetail& detail{ *expected[index] };
            EXPECT_EQ(flag.State, detail.State) << row << " " << index;
            EXPECT_EQ(flag.ResetTicks, detail.ResetTicks) << row << " " << index;
            EXPECT_EQ(flag.IsResetRunning, detail.IsResetRunning) << row << " " << index;
            EXPECT_EQ(flag.AlliesInZone, detail.AlliesInZone) << row << " " << index;
            EXPECT_EQ(flag.CarrierRow, detail.State == FlagState::Carried ? 1 : 0xFF) << row << " " << index;
            EXPECT_TRUE(IsSamePoint(flag.Position, detail.State == FlagState::Carried ? m_Actors[1].Position : red.WorldOrigin)) << row << " " << index;
            EXPECT_TRUE(IsSamePoint(flag.StandPosition, detail.Handle == k_FlagRed ? Vec3{ 1.5f, 2.5f, 3.5f } : Vec3{ -1.5f, -2.5f, -3.5f })) << row << " " << index;
        }
    }
}

namespace
{
    struct InvariantCheck
    {
        std::size_t Views{};
        std::size_t Hud{};
        std::size_t Seen{};
        std::size_t Mutual{};
        std::size_t FlagRows{};
        std::size_t Failures{};
        std::string First{};
    };

    constexpr double k_ConeMargin{ 0.01 };
    constexpr double k_Pi{ 3.14159265358979323846 };

    auto Fail(InvariantCheck& check, std::size_t record, std::string text) -> void
    {
        if (check.Failures++ == 0) check.First = std::format("record {}: {}", record, std::move(text));
    }

    // return: The angle in degrees between the aim of the viewer and the line to the other head, 180 when the heads are at the same point.
    auto AngleTo(const View& viewer, const View& other) -> double
    {
        const Vec3 aim{ AimDirectionOf(viewer) };
        const double x{ static_cast<double>(other.Self.Eye.X) - viewer.Self.Eye.X };
        const double y{ static_cast<double>(other.Self.Eye.Y) - viewer.Self.Eye.Y };
        const double z{ static_cast<double>(other.Self.Eye.Z) - viewer.Self.Eye.Z };
        const double length{ std::sqrt((x * x) + (y * y) + (z * z)) };
        if (length <= 0.0) return 180.0;

        const double cosine{ ((aim.X * x) + (aim.Y * y) + (aim.Z * z)) / length };
        return std::acos((std::max)(-1.0, (std::min)(1.0, cosine))) * 180.0 / k_Pi;
    }

    auto DistanceTo(const View& viewer, const View& other) -> double
    {
        const double x{ static_cast<double>(other.Self.Eye.X) - viewer.Self.Eye.X };
        const double y{ static_cast<double>(other.Self.Eye.Y) - viewer.Self.Eye.Y };
        const double z{ static_cast<double>(other.Self.Eye.Z) - viewer.Self.Eye.Z };
        return std::sqrt((x * x) + (y * y) + (z * z));
    }

    // Checks I1, I2, I3, I5 and I6 of the design on the Views of a tick, with every player an agent.
    auto CheckInvariants(const Chain& chain, std::size_t record, InvariantCheck& check) -> void
    {
        const auto views{ chain.m_Egocentric.m_ViewsStore.Acquire() };
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
        const auto objectives{ chain.m_Environment.m_ObjectiveDetailsStore.Acquire() };
        if (!views || !players || !entities) return;

        const std::size_t rows{ (std::min)(players->Rows.size(), views->size()) };
        for (std::size_t a{ 0 }; a < rows; ++a)
        {
            const View& viewer{ (*views)[a] };
            if (!viewer.Head.IsComputed) continue;

            ++check.Views;
            for (std::size_t b{ 0 }; b < rows; ++b)
            {
                if (a == b) continue;

                const ViewOther& row{ viewer.Others[b] };
                const Player& other{ players->Rows[b] };
                const View& target{ (*views)[b] };

                // I1: seen means alive, within the range and inside the cone.
                if ((row.Channels & k_Seen) != 0)
                {
                    ++check.Seen;
                    if (!viewer.Head.HasBody || !other.IsAlive()) Fail(check, record, std::format("I1: seen with a dead player, {} sees {}", a, b));
                    if (DistanceTo(viewer, target) > k_SightRange + 1.0e-3) Fail(check, record, std::format("I1: seen past the range, {} sees {}", a, b));
                    if (AngleTo(viewer, target) > k_SightHalfAngle + k_ConeMargin) Fail(check, record, std::format("I1: seen outside the cone, {} sees {}", a, b));
                }

                // I2: with the cones of both on each other and well inside, the line is one and so is what both see.
                const bool isMutual{ viewer.Head.HasBody && target.Head.HasBody && !IsZeroVec(viewer.Self.Eye) && !IsZeroVec(target.Self.Eye) &&
                    DistanceTo(viewer, target) <= k_SightRange - 1.0e-2 && AngleTo(viewer, target) < k_SightHalfAngle - k_ConeMargin &&
                    AngleTo(target, viewer) < k_SightHalfAngle - k_ConeMargin };
                if (isMutual)
                {
                    ++check.Mutual;
                    if (((row.Channels & k_Seen) != 0) != (((*views)[b].Others[a].Channels & k_Seen) != 0)) Fail(check, record, std::format("I2: {} and {}", a, b));
                }

                // I3: an ally is always on the Hud, and where it is while it lives.
                if (row.Relation == ViewRelation::Ally)
                {
                    ++check.Hud;
                    if ((row.Channels & k_Hud) == 0) Fail(check, record, std::format("I3: ally {} of {} without Hud", b, a));
                    if (row.IsAlive != other.IsAlive()) Fail(check, record, std::format("I3: IsAlive of ally {} of {}", b, a));
                    const Entity* biped{ entities->Find(other.BipedHandle) };
                    if (other.IsAlive() && biped && !IsSamePoint(row.Position, biped->WorldOrigin)) Fail(check, record, std::format("I3: Position of ally {} of {}", b, a));
                }

                // I5: no channel, no data.
                if (row.Channels == 0 && !IsZeroButRelation(row)) Fail(check, record, std::format("I5: row {} of {}", b, a));
            }

            // I6: a flag row is the flag of the side (or a rival one) as ObjectiveDetails has it, and its position the entity's or the carrier's.
            if (!objectives) continue;

            for (std::size_t index{ 0 }; index < viewer.Flags.size(); ++index)
            {
                std::vector<const ObjectiveDetail*> candidates{};
                for (const auto& [handle, flag] : objectives->Flags)
                {
                    const bool isMatch{ viewer.Head.Team != Team::Neutral && (index == 0 ? flag.OwnerTeam == viewer.Head.Team : (flag.OwnerTeam != viewer.Head.Team && flag.OwnerTeam != Team::Neutral)) };
                    if (isMatch) candidates.push_back(&flag);
                }

                const ViewFlag& row{ viewer.Flags[index] };
                if (candidates.empty())
                {
                    if (!IsZeroFlag(row)) Fail(check, record, std::format("I6: row {} of {} is not zero", index, a));
                    continue;
                }

                ++check.FlagRows;
                const bool isFound{ std::ranges::any_of(candidates, [&](const ObjectiveDetail* flag) {
                    const Entity* carrier{ flag->State == FlagState::Carried ? entities->Find(flag->CarrierBiped) : nullptr };
                    const Entity* where{ carrier ? carrier : entities->Find(flag->Handle) };
                    return row.State == flag->State && row.ResetTicks == flag->ResetTicks && row.IsResetRunning == flag->IsResetRunning &&
                        row.AlliesInZone == flag->AlliesInZone && (!where || IsSamePoint(row.Position, where->WorldOrigin));
                }) };
                if (!isFound) Fail(check, record, std::format("I6: row {} of {} is none of the flags", index, a));
            }
        }
    }

    // Runs the film through the pipeline, every record, with every player an agent.
    auto CheckFilm(std::string_view film, InvariantCheck& check) -> FilmOutcome
    {
        return RunFilmPass(film, false, [&check](const Chain& chain, std::size_t record) { CheckInvariants(chain, record, check); });
    }
}

// I1, I2, I3, I5 and I6 on every tick of ctf_c and ctf_d with every player an agent. Out of the suite: each film costs far more than 10 s. By hand:
// python Tools/test.py --disabled --filter "DISABLED_ViewSightReplayTests.*".
TEST(DISABLED_ViewSightReplayTests, InvariantsHoldInCtfCAndCtfD)
{
    for (const std::string_view film : { "ctf_c", "ctf_d" })
    {
        InvariantCheck check{};
        const FilmOutcome outcome{ CheckFilm(film, check) };
        if (!outcome.Skip.empty()) GTEST_SKIP() << outcome.Skip;

        EXPECT_TRUE(outcome.Error.empty()) << outcome.Error;
        EXPECT_GT(check.Views, 0u) << film;
        EXPECT_EQ(check.Failures, 0u) << film << ": " << check.First;
        std::println("{}: {} records, {} Views, {} Hud rows, {} Seen rows, {} mutual pairs, {} flag rows, {} failures, {:.1f} s.", film, outcome.Records,
            check.Views, check.Hud, check.Seen, check.Mutual, check.FlagRows, check.Failures, outcome.Seconds);
    }
}