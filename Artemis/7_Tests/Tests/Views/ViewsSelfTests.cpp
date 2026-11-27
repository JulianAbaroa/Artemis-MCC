#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Details.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
import Views.View.Type;
import Views.View.System;
import Tests.Support.Bench;
import Tests.Support.FilmPass;
import Tests.Support.Pipeline;
import std;

// The Views of the agents (F4.1, D307): one per row of Players, the head and Self copied from Players, Entities and WeaponDetails.

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::FlagState;
    using Common::Team::Type::Team;
    using Blam::MapModel::Definitions::Type::Bipd::Bipd;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Biped::ZoomLevel;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::StandDetail;
    using Perception::Details::Type::WeaponDetail;
    using Perception::Details::Type::WeaponDetails;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Tests::Support::Bench::Series;
    using Tests::Support::Bench::Stats;
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Pipeline::Chain;
    using Views::View::Type::AgentSet;
    using Views::View::Type::View;
    using Views::View::Type::ViewMode;
    using Views::View::Type::ViewTable;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;

    // Handles whose low 16 bits are the player table index (1, 2 and 5) and the biped and weapons they own.
    constexpr std::uint32_t k_PlayerA{ 0xE5A10001 };
    constexpr std::uint32_t k_PlayerB{ 0xE5A20002 };
    constexpr std::uint32_t k_PlayerC{ 0xE5A30005 };
    constexpr std::uint32_t k_BipedA{ 0xEC010011 };
    constexpr std::uint32_t k_Primary{ 0xAA010021 };
    constexpr std::uint32_t k_Secondary{ 0xAA020022 };
    constexpr std::uint32_t k_Rival{ 0xAA030023 };

    constexpr std::string_view k_SpartanTag{ "test\\spartan" };

    auto MakePlayer(std::uint32_t handle, Team team, bool isAlive) -> Player
    {
        Player player{};
        player.Handle = handle;
        player.Team = team;
        if (isAlive) player.Body = Body{};
        return player;
    }

    auto MakePlayers(std::vector<Player> rows) -> Players
    {
        std::ranges::sort(rows, {}, &Player::Handle);
        Players players{};
        players.Rows = std::move(rows);
        for (std::size_t row{ 0 }; row < players.Rows.size(); ++row) players.Index[players.Rows[row].Handle] = row;
        return players;
    }

    // return: Whether the View is the zero one, field by field of what the tests give a value to.
    auto IsZero(const View& view) -> bool
    {
        return view.Head.PlayerHandle == 0 && view.Head.PlayerRow == 0 && view.Head.Mode == ViewMode::Full && !view.Head.IsComputed &&
            !view.Head.HasBody && view.Head.Team == Team::Red && view.Head.Side == Team::Red &&
            view.Self.RespawnTicks == 0 && view.Self.Ability == 0 && view.Self.Weapons[0].Handle == 0 && view.Self.Weapons[1].Handle == 0 &&
            view.Self.Feet.X == 0.0f && view.Self.Eye.Z == 0.0f && view.Self.Vitality == 0.0f && view.Self.TicksSinceDamage == 0 &&
            view.AimHit.Distance == 0.0f && view.Rays[0].Distance == 0.0f && view.Others[0].Relation == Views::View::Type::ViewRelation::None &&
            view.SoundCount == 0 && view.NoticeCount == 0;
    }

    class ViewsSelfTests : public ::testing::Test, protected Chain
    {
    protected:
        auto PublishPlayers(std::vector<Player> rows) -> void
        {
            m_Environment.m_PlayersStore.Publish(MakePlayers(std::move(rows)));
        }

        auto PublishStands(Team ownerOfFirst, Team ownerOfSecond) -> void
        {
            ObjectiveDetails objectives{};
            objectives.Stands[0xB0000001] = StandDetail{ .Handle = 0xB0000001, .BaseTeam = Team::Red, .OwnerTeam = ownerOfFirst };
            objectives.Stands[0xB0000002] = StandDetail{ .Handle = 0xB0000002, .BaseTeam = Team::Blue, .OwnerTeam = ownerOfSecond };
            m_Environment.m_ObjectiveDetailsStore.Publish(std::move(objectives));
        }

        auto Compute(const AgentSet& agents) -> ViewTable
        {
            m_Egocentric.m_ViewsService.Update(agents);
            const auto table{ m_Egocentric.m_ViewsStore.Acquire() };
            return table ? *table : ViewTable{};
        }
    };
}

TEST_F(ViewsSelfTests, OneViewPerPlayerRowInOrder)
{
    this->PublishPlayers({ MakePlayer(k_PlayerC, Team::Red, true), MakePlayer(k_PlayerA, Team::Blue, true), MakePlayer(k_PlayerB, Team::Red, false) });
    const Players players{ *m_Environment.m_PlayersStore.Acquire() };
    ASSERT_EQ(players.Rows.size(), 3u);

    const ViewTable views{ this->Compute(AgentSet::All()) };
    for (std::size_t row{ 0 }; row < players.Rows.size(); ++row)
    {
        const Player& player{ players.Rows[row] };
        EXPECT_EQ(views[row].Head.PlayerHandle, player.Handle) << row;
        EXPECT_EQ(views[row].Head.PlayerRow, row);
        EXPECT_TRUE(views[row].Head.IsComputed) << row;
        EXPECT_EQ(views[row].Head.Mode, ViewMode::HumanLike);
        EXPECT_EQ(views[row].Head.HasBody, player.IsAlive()) << row;
        EXPECT_EQ(views[row].Head.Team, player.Team) << row;
    }

    // Rows are in handle order, so the player of the lowest handle is View 0, whatever the order they were given in.
    EXPECT_EQ(views[0].Head.PlayerHandle, k_PlayerA);
    for (std::size_t row{ players.Rows.size() }; row < k_MaxPlayers; ++row) EXPECT_TRUE(IsZero(views[row])) << row;
}

TEST_F(ViewsSelfTests, OutsideAgentSetIsZero)
{
    this->PublishPlayers({ MakePlayer(k_PlayerA, Team::Blue, true), MakePlayer(k_PlayerB, Team::Red, true), MakePlayer(k_PlayerC, Team::Red, true) });

    AgentSet agents{};
    agents.Add(k_PlayerB);
    const ViewTable views{ this->Compute(agents) };

    EXPECT_TRUE(IsZero(views[0]));
    EXPECT_TRUE(views[1].Head.IsComputed);
    EXPECT_EQ(views[1].Head.PlayerHandle, k_PlayerB);
    EXPECT_TRUE(IsZero(views[2]));

    // An empty set computes nothing, and a tick without players publishes only zero Views.
    for (const View& view : this->Compute(AgentSet{})) EXPECT_TRUE(IsZero(view));
    m_Environment.m_PlayersStore.Cleanup();
    for (const View& view : this->Compute(AgentSet::All())) EXPECT_TRUE(IsZero(view));
}

TEST_F(ViewsSelfTests, SelfCopiesPlayerEntityAndWeapons)
{
    Bipd bipd{};
    bipd.StandingCameraHeight = 0.62f;
    bipd.CrouchingCameraHeight = 0.42f;
    m_Resolved.m_DefinitionsStore.m_Bipd.Add(std::string{ k_SpartanTag }, bipd);
    m_Resolved.m_DefinitionsStore.Freeze();

    AliveObject biped{};
    biped.Handle = k_BipedA;
    biped.TagName = std::string{ k_SpartanTag };
    biped.WorldOrigin = { 10.0f, 20.0f, 30.0f };
    biped.Up = { 0.0f, 0.0f, 1.0f };
    m_Tables.m_ObjectStore.AddObject(k_BipedA, biped);
    m_Tables.m_ObjectStore.Publish();

    Player player{ MakePlayer(k_PlayerA, Team::Blue, true) };
    player.BipedHandle = k_BipedA;
    player.RespawnTicks = 0;
    player.PowerupTicks = 77;
    Body& body{ *player.Body };
    body.Aim = { 0.0f, 1.0f, 0.0f };
    body.Zoom = ZoomLevel::Zoom1;
    body.TicksAlive = 321;
    body.Shield = 0.5f;
    body.OwnVelocity = { 1.0f, 2.0f, 3.0f };
    body.CrouchFraction = 0.0f;
    body.TicksSinceDamage = 9;
    body.AirborneTicks = 4;
    body.Loadout.PrimaryWeapon = k_Primary;
    body.Loadout.SecondaryWeapon = k_Secondary;
    body.Loadout.EquippedWeapon = k_Primary;
    body.Loadout.FragGrenades = 2;
    body.Loadout.PlasmaGrenades = 1;
    body.Loadout.AbilityEnergy = 0.25f;
    body.Loadout.Objective = 0xF1A60001;
    this->PublishPlayers({ player });

    Entities entities{};
    Entity bipedEntity{};
    bipedEntity.Handle = k_BipedA;
    bipedEntity.WorldOrigin = biped.WorldOrigin;
    bipedEntity.LinearVelocity = { 4.0f, 5.0f, 6.0f };
    bipedEntity.Vitality = 0.75f;
    Entity primaryEntity{};
    primaryEntity.Handle = k_Primary;
    primaryEntity.Subtype = 5;
    Entity secondaryEntity{};
    secondaryEntity.Handle = k_Secondary;
    secondaryEntity.Subtype = 7;
    entities.Rows = { primaryEntity, secondaryEntity, bipedEntity };
    std::ranges::sort(entities.Rows, {}, &Entity::Handle);
    m_Environment.m_EntitiesStore.Publish(std::move(entities));

    WeaponDetails weapons{};
    WeaponDetail primary{};
    primary.Handle = k_Primary;
    primary.MagazineCount = 1;
    primary.Magazines[0] = { .Loaded = 8, .Reserve = 40 };
    primary.Heat = 0.3f;
    primary.CanFireNow = true;
    weapons[k_Primary] = primary;
    WeaponDetail secondary{};
    secondary.Handle = k_Secondary;
    secondary.IsEnergyWeapon = true;
    secondary.Energy = 0.6f;
    secondary.IsOverheated = true;
    secondary.ChargeFraction = 0.5f;
    weapons[k_Secondary] = secondary;
    WeaponDetail rival{};
    rival.Handle = k_Rival;
    rival.TrackedObject = k_BipedA;
    rival.IsLocked = true;
    weapons[k_Rival] = rival;
    m_Environment.m_WeaponDetailsStore.Publish(std::move(weapons));

    const View view{ this->Compute(AgentSet::All())[0] };
    const auto& self{ view.Self };
    EXPECT_TRUE(view.Head.HasBody);
    EXPECT_FLOAT_EQ(self.Eye.X, 10.0f);
    EXPECT_FLOAT_EQ(self.Eye.Y, 20.0f);
    EXPECT_FLOAT_EQ(self.Eye.Z, 30.62f);
    EXPECT_EQ(self.Feet.X, 10.0f);
    EXPECT_EQ(self.Feet.Z, 30.0f);
    EXPECT_NEAR(self.FrameYaw, std::numbers::pi_v<float> / 2.0f, 1.0e-6f);
    EXPECT_NEAR(self.AimPitch, 0.0f, 1.0e-6f);
    EXPECT_EQ(self.LinearVelocity.Y, 5.0f);
    EXPECT_EQ(self.OwnVelocity.Z, 3.0f);
    EXPECT_EQ(self.Vitality, 0.75f);
    EXPECT_EQ(self.Shield, 0.5f);
    EXPECT_EQ(self.Zoom, ZoomLevel::Zoom1);
    EXPECT_EQ(self.TicksAlive, 321u);
    EXPECT_EQ(self.PowerupTicks, 77u);
    EXPECT_EQ(self.AirborneTicks, 4u);
    EXPECT_EQ(self.TicksSinceDamage, 9u);
    EXPECT_EQ(self.FragGrenades, 2);
    EXPECT_EQ(self.PlasmaGrenades, 1);
    EXPECT_EQ(self.AbilityEnergy, 0.25f);
    EXPECT_EQ(self.Ability, k_NoHandle);
    EXPECT_TRUE(self.CarriesFlag);
    EXPECT_TRUE(self.IsTracked);

    EXPECT_EQ(self.Weapons[0].Handle, k_Primary);
    EXPECT_EQ(self.Weapons[0].Subtype, 5);
    EXPECT_TRUE(self.Weapons[0].IsEquipped);
    EXPECT_EQ(self.Weapons[0].Loaded, 8);
    EXPECT_EQ(self.Weapons[0].Reserve, 40);
    EXPECT_EQ(self.Weapons[0].Heat, 0.3f);
    EXPECT_TRUE(self.Weapons[0].CanFireNow);
    EXPECT_TRUE(std::isnan(self.Weapons[0].Energy));
    EXPECT_EQ(self.Weapons[1].Handle, k_Secondary);
    EXPECT_EQ(self.Weapons[1].Subtype, 7);
    EXPECT_FALSE(self.Weapons[1].IsEquipped);
    EXPECT_EQ(self.Weapons[1].Energy, 0.6f);
    EXPECT_TRUE(self.Weapons[1].IsOverheated);
    EXPECT_EQ(self.Weapons[1].ChargeFraction, 0.5f);

    // The sections that later tasks fill are still zero.
    EXPECT_EQ(view.AimHit.Distance, 0.0f);
    EXPECT_EQ(view.SoundCount, 0);
    EXPECT_EQ(view.NoticeCount, 0);
}

TEST_F(ViewsSelfTests, DeadPlayerHasNoBodyButKeepsRespawnAndSide)
{
    Player player{ MakePlayer(k_PlayerA, Team::Blue, false) };
    player.RespawnTicks = 120;
    player.BipedHandle = k_BipedA;
    this->PublishPlayers({ player });
    this->PublishStands(Team::Blue, Team::Red);

    const View view{ this->Compute(AgentSet::All())[0] };
    EXPECT_TRUE(view.Head.IsComputed);
    EXPECT_FALSE(view.Head.HasBody);
    EXPECT_EQ(view.Head.Side, Team::Red);
    EXPECT_EQ(view.Self.RespawnTicks, 120u);
    EXPECT_EQ(view.Self.Weapons[0].Handle, k_NoHandle);
    EXPECT_EQ(view.Self.Weapons[1].Handle, k_NoHandle);
    EXPECT_EQ(view.Self.Ability, k_NoHandle);
    EXPECT_EQ(view.Self.TicksSinceDamage, k_NoHandle);
    EXPECT_EQ(view.Self.Zoom, ZoomLevel::None);
    EXPECT_EQ(view.Self.Feet.X, 0.0f);
    EXPECT_EQ(view.Self.Vitality, 0.0f);
    EXPECT_FALSE(view.Self.IsTracked);
}

TEST_F(ViewsSelfTests, SideFollowsOwnerTeamOfTheRound)
{
    this->PublishPlayers({ MakePlayer(k_PlayerA, Team::Blue, true), MakePlayer(k_PlayerB, Team::Red, true), MakePlayer(k_PlayerC, Team::Green, true) });

    // Round one: blue plays from the stand of BaseTeam red, red from the stand of BaseTeam blue. Green owns none.
    this->PublishStands(Team::Blue, Team::Red);
    ViewTable views{ this->Compute(AgentSet::All()) };
    EXPECT_EQ(views[0].Head.Side, Team::Red);
    EXPECT_EQ(views[1].Head.Side, Team::Blue);
    EXPECT_EQ(views[2].Head.Side, Team::Neutral);

    // The next round switches the sides.
    this->PublishStands(Team::Red, Team::Blue);
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(views[0].Head.Side, Team::Blue);
    EXPECT_EQ(views[1].Head.Side, Team::Red);
    EXPECT_EQ(views[2].Head.Side, Team::Neutral);

    // Without objectives (a game that is not capture the flag) nobody has a side.
    m_Environment.m_ObjectiveDetailsStore.Cleanup();
    views = this->Compute(AgentSet::All());
    EXPECT_EQ(views[0].Head.Side, Team::Neutral);
}

namespace
{
    // The bits of a value, to compare it with another without the NaN of Shield or Energy breaking the comparison.
    template <typename T>
    auto SameBits(const T& a, const T& b) -> bool
    {
        return std::memcmp(&a, &b, sizeof(T)) == 0;
    }

    struct ReplayCheck
    {
        std::size_t Views{};
        std::size_t WithBody{};
        std::size_t Mismatches{};
        std::string First{};
    };

    // Compares Self with the Player, Entity and weapons of the same handle, bit by bit (I7 of the design).
    auto CheckSelves(const Chain& chain, ReplayCheck& check) -> void
    {
        const auto views{ chain.m_Egocentric.m_ViewsStore.Acquire() };
        const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
        const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
        const auto weapons{ chain.m_Environment.m_WeaponDetailsStore.Acquire() };
        if (!views || !players || !entities || !weapons) return;

        const auto fail{ [&check](std::size_t row, std::string_view field) {
            if (check.Mismatches++ == 0) check.First = std::format("row {}: {}", row, field);
        } };

        for (std::size_t row{ 0 }; row < players->Rows.size(); ++row)
        {
            const Player& player{ players->Rows[row] };
            const View& view{ (*views)[row] };
            ++check.Views;
            if (!view.Head.IsComputed || view.Head.PlayerHandle != player.Handle || view.Head.PlayerRow != row) fail(row, "head");
            if (view.Self.RespawnTicks != player.RespawnTicks || view.Head.HasBody != player.IsAlive()) fail(row, "respawn or body");
            if (!player.Body) continue;

            ++check.WithBody;
            const Body& body{ *player.Body };
            const auto& self{ view.Self };
            if (!SameBits(self.Shield, body.Shield.value_or(std::numeric_limits<float>::quiet_NaN()))) fail(row, "Shield");
            if (!SameBits(self.OwnVelocity, body.OwnVelocity)) fail(row, "OwnVelocity");
            if (!SameBits(self.CrouchFraction, body.CrouchFraction)) fail(row, "CrouchFraction");
            if (self.Zoom != body.Zoom || self.TicksAlive != body.TicksAlive || self.AirborneTicks != body.AirborneTicks) fail(row, "Zoom, TicksAlive or AirborneTicks");
            if (self.Ground != body.Ground || self.IsGrounded != body.IsGrounded) fail(row, "Ground");
            if (self.FragGrenades != body.Loadout.FragGrenades || self.PlasmaGrenades != body.Loadout.PlasmaGrenades || self.SelectedGrenade != body.Loadout.SelectedGrenade) fail(row, "grenades");
            if (self.Ability != body.Loadout.Ability || !SameBits(self.AbilityEnergy, body.Loadout.AbilityEnergy) || self.IsAbilityActive != body.Loadout.IsAbilityActive) fail(row, "ability");
            if (self.CarriesFlag != (body.Loadout.Objective != k_NoHandle)) fail(row, "CarriesFlag");
            if (self.TicksSinceDamage != body.TicksSinceDamage.value_or(k_NoHandle)) fail(row, "TicksSinceDamage");
            if (self.Powerup != player.Powerup || self.PowerupTicks != player.PowerupTicks) fail(row, "Powerup");
            if (!SameBits(self.FrameYaw, std::atan2(body.Aim.Y, body.Aim.X))) fail(row, "FrameYaw");
            if (!SameBits(self.AimPitch, std::atan2(body.Aim.Z, std::hypot(body.Aim.X, body.Aim.Y)))) fail(row, "AimPitch");

            const Entity* entity{ entities->Find(player.BipedHandle) };
            if (entity)
            {
                if (!SameBits(self.Feet, entity->WorldOrigin) || !SameBits(self.LinearVelocity, entity->LinearVelocity)) fail(row, "Feet or LinearVelocity");
                if (!SameBits(self.Vitality, entity->Vitality.value_or(std::numeric_limits<float>::quiet_NaN()))) fail(row, "Vitality");
            }

            const std::array<std::uint32_t, 2> slots{ body.Loadout.PrimaryWeapon, body.Loadout.SecondaryWeapon };
            for (std::size_t slot{ 0 }; slot < slots.size(); ++slot)
            {
                const auto& weapon{ self.Weapons[slot] };
                if (weapon.Handle != slots[slot]) fail(row, "weapon handle");
                if (slots[slot] == k_NoHandle) continue;

                if (weapon.IsEquipped != (slots[slot] == body.Loadout.EquippedWeapon)) fail(row, "IsEquipped");
                const Entity* weaponEntity{ entities->Find(slots[slot]) };
                if (weaponEntity && weapon.Subtype != weaponEntity->Subtype) fail(row, "weapon Subtype");
                const auto detail{ weapons->find(slots[slot]) };
                if (detail == weapons->end()) continue;

                const auto& d{ detail->second };
                if (!SameBits(weapon.Energy, d.Energy) || !SameBits(weapon.Heat, d.Heat) || !SameBits(weapon.ChargeFraction, d.ChargeFraction)) fail(row, "weapon floats");
                if (weapon.IsOverheated != d.IsOverheated || weapon.IsReloading != d.IsReloading || weapon.CanFireNow != d.CanFireNow) fail(row, "weapon flags");
                if (d.MagazineCount > 0 && (weapon.Loaded != d.Magazines[0].Loaded || weapon.Reserve != d.Magazines[0].Reserve)) fail(row, "weapon rounds");
            }
        }

        // Rows past the players are zero.
        for (std::size_t row{ players->Rows.size() }; row < k_MaxPlayers; ++row)
        {
            if ((*views)[row].Head.IsComputed) fail(row, "empty row");
        }
    }
}

namespace
{
    // Runs the film through the pipeline with every player an agent and checks I7 on the records it samples.
    auto CheckFilm(std::string_view film, double maxSeconds) -> void
    {
        ReplayCheck check{};

        // One record in 256 goes through the pipeline: the check does not depend on history. The time is the reading of the .sdump and the map.
        const FilmOutcome outcome{ RunFilmPass(film, false, [&check](const Chain& chain, std::size_t) { CheckSelves(chain, check); }, {}, 256) };
        if (!outcome.Skip.empty()) GTEST_SKIP() << outcome.Skip;

        EXPECT_TRUE(outcome.Error.empty()) << outcome.Error;
        EXPECT_GT(check.Views, 0u);
        EXPECT_GT(check.WithBody, 0u);
        EXPECT_EQ(check.Mismatches, 0u) << check.First;
        EXPECT_LT(outcome.Seconds, maxSeconds);
        std::println("{}: {} records, {} Views checked, {} with body, {:.1f} s.", film, outcome.Records, check.Views, check.WithBody, outcome.Seconds);
    }
}

// I7 on ctf_c. Any replay costs about 17 s of map load and .sdump reading, so it runs by hand: python Tools/test.py --disabled --filter "DISABLED_ViewsReplayTests.*".
TEST(DISABLED_ViewsReplayTests, SelfMatchesPlayersEntitiesAndWeaponsInCtfC)
{
    CheckFilm("ctf_c", 120.0);
}

// Cost of ViewsService::Update with 16 agents over the players of Alien Landing 8v8 (F4.1), in microseconds per decision. F4.B measured the
// assembly of 16 Views at 9 to 15 us: this is the same kind of number now that Self is a copy of the stores. The table goes to
// build/bench/views_update.txt. By hand: python Tools/test.py --disabled --filter "DISABLED_ViewsBench.*".
TEST(DISABLED_ViewsBench, Update16Agents)
{
    Series update{};
    Series computed{};

    // One record in 8 is timed: a decision every four ticks would be 1 in 4 and the cost does not depend on history.
    const FilmOutcome outcome{ RunFilmPass("BenchFilms/alien8v8", false, [&](const Chain& chain, std::size_t)
    {
        // note: The chain is the one of this pass and Update only publishes again the Views the pipeline just made.
        auto& service{ const_cast<Chain&>(chain).m_Egocentric.m_ViewsService };
        const auto start{ std::chrono::steady_clock::now() };
        service.Update(AgentSet::All());
        const auto end{ std::chrono::steady_clock::now() };
        update.Add(std::chrono::duration<double, std::micro>(end - start).count());

        const auto views{ chain.m_Egocentric.m_ViewsStore.Acquire() };
        double count{ 0.0 };
        if (views)
        {
            for (const View& view : *views) count += view.Head.IsComputed ? 1.0 : 0.0;
        }
        computed.Add(count);
    }, {}, 8) };
    if (!outcome.Skip.empty()) GTEST_SKIP() << outcome.Skip;

    const Stats cost{ update.Summary() };
    const Stats agents{ computed.Summary() };
    std::vector<std::string> report{};
    report.push_back(std::format("ViewsService::Update over alien8v8, {}.", Tests::Support::Bench::Configuration()));
    report.push_back(std::format("sizeof: View {}, ViewTable {}, Head {}, Self {}, Weapon {}, AimHit {}, Ray {}, Other {}, Flag {}, Sound {}, Notice {}.", sizeof(View), sizeof(ViewTable),
        sizeof(Views::View::Type::ViewHead), sizeof(Views::View::Type::ViewSelf), sizeof(Views::View::Type::ViewWeapon), sizeof(Views::View::Type::ViewAimHit),
        sizeof(Views::View::Type::ViewRayHit), sizeof(Views::View::Type::ViewOther), sizeof(Views::View::Type::ViewFlag), sizeof(Views::View::Type::ViewSound),
        sizeof(Views::View::Type::ViewNotice)));
    report.push_back(std::format("agents computed: mean {:.1f}, max {:.0f}.", agents.Mean, agents.Max));
    report.push_back(std::format("Update, all players: n={} mean={:.1f} p50={:.1f} p99={:.1f} max={:.1f} us.", cost.N, cost.Mean, cost.P50, cost.P99, cost.Max));
    for (const std::string& line : report) std::println("{}", line);
    EXPECT_FALSE(Tests::Support::Bench::WriteReport("views_update.txt", report).empty());
    EXPECT_GT(cost.N, 0u);
}

// D385: who gets a View without and with a Python client that owns the instance.
TEST(AgentSetOfTests, TheMaskComesFromTheOwnerAndTheWishOfTheViewer)
{
    using Views::View::System::AgentSetOf;

    // A client that owns the instance chooses, whatever the Viewer wants.
    EXPECT_EQ(AgentSetOf(true, 0b101u, true).Mask, 0b101u);
    EXPECT_EQ(AgentSetOf(true, 0b101u, false).Mask, 0b101u);

    // Without a client: every player when the Viewer wants all the Views, else none (the caller adds the local and the observed player).
    EXPECT_EQ(AgentSetOf(false, 0b101u, true).Mask, 0xFFFFFFFFu);
    EXPECT_EQ(AgentSetOf(false, 0b101u, false).Mask, 0u);
}