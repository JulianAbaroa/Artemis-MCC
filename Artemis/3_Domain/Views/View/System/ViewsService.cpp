module Views.View.System;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Math.System;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.World.System;
import Blam.Runtime.Object.Type;
import Perception.Details.Type;
import Perception.Entities.Type;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;
import Views.Raycast.Type;
import Views.Raycast.State;
import Views.Raycast.System;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Binding;
    using Common::Semantic::Type::FlagState;
    using Common::Team::Type::Team;
    using Blam::MapModel::Definitions::Type::Bipd::Bipd;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Blam::Runtime::Object::Type::Biped::ZoomLevel;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::StandDetail;
    using Perception::Details::Type::WeaponDetail;
    using Perception::Details::Type::WeaponDetails;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Blam::MapModel::Definitions::State::DefinitionsStore;
    using Blam::MapModel::Definitions::Type::Weap::Weap;
    using Blam::MapModel::World::System::Raycaster;
    using Perception::Players::Type::Loadout;
    using Perception::Players::Type::Players;
    using Perception::Players::Type::Body;
    using Views::Raycast::State::RaycastSceneStore;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::PlayerExclusions;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneReach;
    using Views::Raycast::Type::SelfExclusion;
    using Views::Raycast::Type::SightCounters;
    using Views::Raycast::Type::SightPlayer;
    using Views::Raycast::Type::SightPlayers;
    using Views::Raycast::Type::SightSeen;
    using Views::Raycast::Type::Pattern::PatternRay;
    using Views::Raycast::Type::Pattern::k_PatternRays;
    using Views::Raycast::Type::Constant::k_FallbackAimRange;
    using Views::Raycast::Type::Constant::k_MaxRayRange;
    using Views::Raycast::Type::Raycasts;
    using Views::View::Type::ViewAimHit;
    using Views::View::Type::ViewChannel;
    using Views::View::Type::ViewFlag;
    using Views::View::Type::ViewOther;
    using Views::View::Type::ViewTable;
    using Views::View::Type::ViewRayHit;
    using Views::View::Type::ViewRelation;
    using Perception::Players::Type::Player;
    using Views::Raycast::System::BuildGraphExclusions;
    using Views::Raycast::System::ComputeSight;
    using Views::Raycast::System::CastSceneRay;
    using Views::Raycast::System::CastSceneRayReach;
    using Views::Raycast::System::DerivedEye;
    using Views::Raycast::System::FindVehicleRoot;
    using Views::Raycast::System::GatherSceneReach;
    using Views::Raycast::System::PrepareScene;
    using Views::View::Type::View;
    using Views::View::Type::ViewMode;
    using Views::View::Type::ViewSelf;
    using Views::View::Type::ViewTable;
    using Views::View::Type::ViewWeapon;
    using Views::View::Type::k_ViewWeaponSlots;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_PrimaryLocalUser;

    constexpr std::uint8_t k_NoRow{ 0xFF };
    constexpr double k_Pi{ 3.14159265358979323846 };

    // Autoaim range of a weapon datum, by the DatumIndex of its tag: a small list kept for one Update, so the tag name is looked up once per
    // kind of weapon and not once per agent.
    struct AimRanges : std::vector<std::pair<std::uint32_t, float>>
    {
        using vector::vector;
    };

    // return: The aim range of the primary weapon of the loadout, capped, or the fallback range without a weapon or a definition.
    auto AimRangeOf(const Loadout& loadout, const ObjectTable& objects, const DefinitionsStore& definitions, AimRanges& known) -> float
    {
        if (loadout.PrimaryWeapon == 0 || loadout.PrimaryWeapon == k_NoHandle) return k_FallbackAimRange;

        const auto weapon{ objects.find(loadout.PrimaryWeapon) };
        if (weapon == objects.end()) return k_FallbackAimRange;

        const std::uint32_t datum{ weapon->second.DatumIndex };
        const auto cached{ std::ranges::find(known, datum, &std::pair<std::uint32_t, float>::first) };
        if (cached != known.end()) return cached->second;

        const Weap* weap{ definitions.m_Weap.IsFrozen() ? definitions.m_Weap.Get(weapon->second.TagName) : nullptr };
        const float range{ (std::min)((weap && weap->AutoaimRange > 0.0f) ? weap->AutoaimRange : k_FallbackAimRange, k_MaxRayRange) };
        known.emplace_back(datum, range);
        return range;
    }

    // return: How the team of the agent relates to the object a ray hit: Neutral when the object has no team or the agent has none.
    auto RelationOf(Team agent, const Entity* entity) -> ViewRelation
    {
        if (!entity || entity->Team == Team::Neutral || agent == Team::Neutral) return ViewRelation::Neutral;
        return entity->Team == agent ? ViewRelation::Ally : ViewRelation::Enemy;
    }

    // return: The direction of a ray of the pattern in the frame of the agent: forward, left and up of the frame.
    auto PatternDirection(const PatternRay& ray, const Vec3& forward, const Vec3& left, const Vec3& up) -> Vec3
    {
        const double yaw{ ray.Yaw * k_Pi / 180.0 };
        const double pitch{ ray.Pitch * k_Pi / 180.0 };
        const double a{ std::cos(pitch) * std::cos(yaw) };
        const double b{ std::cos(pitch) * std::sin(yaw) };
        const double c{ std::sin(pitch) };
        return Vec3{
            static_cast<float>(forward.X * a + left.X * b + up.X * c),
            static_cast<float>(forward.Y * a + left.Y * b + up.Y * c),
            static_cast<float>(forward.Z * a + left.Z * b + up.Z * c) };
    }

    // return: The direction of a ray of the pattern in the frame of the aim (D311): forward is the heading on the ground plane, up the world up.
    auto PatternDirectionAt(const PatternRay& ray, float heading) -> Vec3
    {
        const Vec3 forward{ std::cos(heading), std::sin(heading), 0.0f };
        const Vec3 left{ -std::sin(heading), std::cos(heading), 0.0f };
        return PatternDirection(ray, forward, left, Vec3{ 0.0f, 0.0f, 1.0f });
    }

    // What the casts of every agent of an Update share.
    struct RayContext
    {
        const Raycaster& Caster;
        const RaycastScene& Scene;
        const PlayerExclusions& Exclusions;
        const Entities& EntityRows;
        const ObjectTable& Objects;
        const Players& PlayerRows;
        const DefinitionsStore& Definitions;
        AimRanges& Ranges;
    };

    auto FillAimHit(ViewAimHit& out, const RaycastHit& hit, const RayContext& context, Team team) -> void
    {
        out.PlayerRow = k_NoRow;
        out.ObjectHandle = k_NoHandle;
        out.RegionIndex = -1;
        out.Material = -1;
        out.MaxDistance = hit.MaxDistance;
        out.Distance = hit.Hit ? hit.Distance : hit.MaxDistance;
        out.Kind = hit.Kind;
        out.SurfaceFlags = hit.SurfaceFlags;
        if (hit.Hit) out.Point = hit.Point;
        if (hit.Hit) out.Material = hit.Material;
        if (!hit.Hit || hit.Kind != HitKind::Dynamic) return;

        out.ObjectHandle = hit.ObjectHandle;
        out.RegionIndex = hit.RegionIndex;
        const Entity* entity{ context.EntityRows.Find(hit.ObjectHandle) };
        if (entity)
        {
            out.Function = entity->Function;
            out.Subtype = entity->Subtype;
        }
        out.Relation = RelationOf(team, entity);

        for (std::size_t row{ 0 }; row < context.PlayerRows.Rows.size(); ++row)
        {
            if (context.PlayerRows.Rows[row].BipedHandle == hit.ObjectHandle) out.PlayerRow = static_cast<std::uint8_t>(row);
        }
    }

    auto FillRayHit(ViewRayHit& out, const RaycastHit& hit, const RayContext& context, Team team) -> void
    {
        out.Distance = hit.Hit ? hit.Distance : hit.MaxDistance;
        out.Kind = hit.Kind;
        out.ObjectHandle = k_NoHandle;
        out.Material = hit.Hit ? hit.Material : std::int16_t{ -1 };
        out.SurfaceFlags = hit.SurfaceFlags;
        if (!hit.Hit || hit.Kind != HitKind::Dynamic) return;

        out.ObjectHandle = hit.ObjectHandle;
        const Entity* entity{ context.EntityRows.Find(hit.ObjectHandle) };
        if (entity) out.Function = entity->Function;
        out.Relation = RelationOf(team, entity);
    }

    // Casts the aim ray and the rays of the pattern of one agent through the shared scene, with the exclusion of its row.
    // note: The aim ray starts at the eye (Eye is zero when the biped has no tag: then it does not fire). The perception rays start at the
    // bounding center of the vehicle the biped rides, or of the biped, and their frame is the heading of the aim (D311).
    auto CastViewRays(View& view, const Player& player, const Vec3& aim, bool hasEye, std::size_t row, const RayContext& context) -> void
    {
        const SelfExclusion& exclusion{ context.Exclusions[row] };
        const float aimRange{ AimRangeOf(player.Body->Loadout, context.Objects, context.Definitions, context.Ranges) };
        if (hasEye) FillAimHit(view.AimHit, CastSceneRay(context.Caster, context.Scene, exclusion, view.Self.Eye, aim, aimRange, nullptr), context, player.Team);

        std::uint32_t target{ player.BipedHandle };
        const std::uint32_t vehicle{ FindVehicleRoot(context.EntityRows, player.BipedHandle) };
        if (vehicle != k_NoHandle) target = vehicle;

        const auto object{ context.Objects.find(target) };
        if (object == context.Objects.end()) return;

        const Vec3 origin{ object->second.BoundingCenter };
        view.RayOrigin = origin;

        float radius{ 0.0f };
        for (const PatternRay& ray : k_PatternRays) radius = (std::max)(radius, ray.Range);
        const SceneReach reach{ GatherSceneReach(context.Scene, exclusion, origin, radius, nullptr) };

        for (std::size_t index{ 0 }; index < k_PatternRays.size(); ++index)
        {
            const PatternRay& ray{ k_PatternRays[index] };
            const Vec3 direction{ PatternDirectionAt(ray, view.Self.FrameYaw) };
            FillRayHit(view.Rays[index], CastSceneRayReach(context.Caster, context.Scene, reach, exclusion, origin, direction, ray.Range, nullptr), context, player.Team);
        }
    }

    // return: The value, or NaN when it is absent (Shield, Vitality), as the bridge writes it.
    auto ValueOrNaN(const std::optional<float>& value) -> float
    {
        return value.value_or(std::numeric_limits<float>::quiet_NaN());
    }

    // return: The stand the team owns in this round (the lowest handle if several, so the answer does not depend on the map order), or the
    // one of a rival team (isOwn false). Null for a team without a side or a game without that stand.
    auto StandOfSide(const ObjectiveDetails& objectives, Team team, bool isOwn) -> const StandDetail*
    {
        if (team == Team::Neutral) return nullptr;

        const StandDetail* best{ nullptr };
        for (const auto& [handle, stand] : objectives.Stands)
        {
            const bool isMatch{ isOwn ? stand.OwnerTeam == team : (stand.OwnerTeam != team && stand.OwnerTeam != Team::Neutral) };
            if (isMatch && (!best || handle < best->Handle)) best = &stand;
        }
        return best;
    }

    // return: The BaseTeam of the stand the team owns in this round, Neutral when it owns none.
    auto SideOf(const ObjectiveDetails& objectives, Team team) -> Team
    {
        const StandDetail* stand{ StandOfSide(objectives, team, true) };
        return stand ? stand->BaseTeam : Team::Neutral;
    }

    auto FillWeapon(ViewWeapon& slot, std::uint32_t handle, std::uint32_t equipped, const Entities* entities,
        const WeaponDetails* weapons) -> void
    {
        slot.Handle = handle;
        if (handle == k_NoHandle) return;

        slot.IsEquipped = handle == equipped;
        if (entities)
        {
            if (const Entity* entity{ entities->Find(handle) }) slot.Subtype = entity->Subtype;
        }

        if (!weapons) return;
        const auto it{ weapons->find(handle) };
        if (it == weapons->end()) return;

        const WeaponDetail& detail{ it->second };
        if (detail.MagazineCount > 0)
        {
            slot.Loaded = detail.Magazines[0].Loaded;
            slot.Reserve = detail.Magazines[0].Reserve;
        }
        slot.Energy = detail.Energy;
        slot.Heat = detail.Heat;
        slot.IsOverheated = detail.IsOverheated;
        slot.IsReloading = detail.IsReloading;
        slot.ChargeFraction = detail.ChargeFraction;
        slot.CanFireNow = detail.CanFireNow;
    }

    // return: The objects that a weapon has locked on, once per tick: nearly always none.
    auto LockedTargets(const WeaponDetails* weapons) -> std::vector<std::uint32_t>
    {
        std::vector<std::uint32_t> targets{};
        if (!weapons) return targets;

        for (const auto& [handle, detail] : *weapons)
        {
            if (detail.IsLocked && detail.TrackedObject != k_NoHandle) targets.push_back(detail.TrackedObject);
        }
        return targets;
    }

    // return: Whether a weapon that tracks the biped, or the vehicle it sits in, has the lock.
    auto IsTrackedBy(const std::vector<std::uint32_t>& locked, const Entity& biped) -> bool
    {
        if (locked.empty()) return false;

        const std::uint32_t vehicle{ biped.Binding == Binding::Seated ? biped.BoundTo : k_NoHandle };
        return std::ranges::any_of(locked, [&biped, vehicle](std::uint32_t target) { return target == biped.Handle || target == vehicle; });
    }

    // return: The derived eye (D309) of the biped of a living player, empty without a body, an object or the definition of its biped.
    auto EyeOf(const Player& player, const ObjectTable& objects, const DefinitionsStore& definitions) -> std::optional<Vec3>
    {
        if (!player.Body) return std::nullopt;

        const auto object{ objects.find(player.BipedHandle) };
        if (object == objects.end()) return std::nullopt;

        const Bipd* definition{ definitions.m_Bipd.Get(object->second.TagName) };
        if (!definition) return std::nullopt;

        return DerivedEye(object->second, *definition, player.Body->CrouchFraction);
    }

    // return: How the team of the agent relates to the team of another player: None when either has no valid team.
    auto RelationBetween(Team agent, Team other) -> ViewRelation
    {
        if (agent == Team::Neutral || other == Team::Neutral) return ViewRelation::None;
        return agent == other ? ViewRelation::Ally : ViewRelation::Enemy;
    }

    // Fills what the agent knows of each row of Players (D308): the allies by the Hud, the living players it sees, and nothing else.
    // note: A row without channel stays zero but for its Relation. The Radar bit and Elevation are left for F4.6.
    auto FillOthers(View& view, const Players& players, const Entities& entities, const SightSeen& seen) -> void
    {
        const std::size_t self{ view.Head.PlayerRow };
        const bool isFull{ view.Head.Mode == ViewMode::Full };
        const std::size_t rows{ (std::min)(players.Rows.size(), k_MaxPlayers) };

        for (std::size_t row{ 0 }; row < rows; ++row)
        {
            const Player& other{ players.Rows[row] };
            ViewOther& out{ view.Others[row] };
            if (row == self)
            {
                out.Relation = ViewRelation::Self;
                continue;
            }

            out.Relation = RelationBetween(view.Head.Team, other.Team);
            const bool isHud{ out.Relation == ViewRelation::Ally };
            const bool isSeen{ other.IsAlive() && (isFull || seen[self][row]) };
            if (!isHud && !isSeen) continue;

            if (isHud) out.Channels |= static_cast<std::uint8_t>(ViewChannel::Hud);
            if (isSeen) out.Channels |= static_cast<std::uint8_t>(ViewChannel::Seen);
            out.IsAlive = other.IsAlive();
            if (!other.IsAlive()) continue;

            const Body& body{ *other.Body };
            const Entity* biped{ entities.Find(other.BipedHandle) };
            out.CarriesFlag = body.Loadout.Objective != k_NoHandle;
            if (biped) out.Position = biped->WorldOrigin;
            if (!isSeen) continue;

            if (biped) out.Velocity = biped->LinearVelocity;
            out.FacingYaw = std::atan2(body.Aim.Y, body.Aim.X);
            out.Vitality = ValueOrNaN(biped ? biped->Vitality : std::nullopt);
            out.Shield = ValueOrNaN(body.Shield);
            out.IsCrouching = body.IsCrouching;
            if (body.Loadout.EquippedWeapon == k_NoHandle) continue;

            if (const Entity* weapon{ entities.Find(body.Loadout.EquippedWeapon) }) out.WeaponSubtype = weapon->Subtype;
        }
    }

    // return: The flag of the team that owns the side (the lowest handle if several), or the one of a rival team (isOwn false). Null for a
    // team without a side or a game without that flag.
    auto FlagOfSide(const ObjectiveDetails& objectives, Team team, bool isOwn) -> const ObjectiveDetail*
    {
        if (team == Team::Neutral) return nullptr;

        const ObjectiveDetail* best{ nullptr };
        for (const auto& [handle, flag] : objectives.Flags)
        {
            const bool isMatch{ isOwn ? flag.OwnerTeam == team : (flag.OwnerTeam != team && flag.OwnerTeam != Team::Neutral) };
            if (isMatch && (!best || handle < best->Handle)) best = &flag;
        }
        return best;
    }

    // Copies a flag, where it is (or where its carrier is), who carries it and where its stand is.
    auto FillFlag(ViewFlag& out, const ObjectiveDetail& flag, const ObjectiveDetails& objectives, const Players& players,
        const Entities& entities) -> void
    {
        out.HasFlag = true;
        out.State = flag.State;
        out.CarrierRow = k_NoRow;
        out.ResetTicks = flag.ResetTicks;
        out.IsResetRunning = flag.IsResetRunning;
        out.AlliesInZone = flag.AlliesInZone;

        const Entity* carrier{ flag.State == FlagState::Carried ? entities.Find(flag.CarrierBiped) : nullptr };
        if (const Entity* where{ carrier ? carrier : entities.Find(flag.Handle) }) out.Position = where->WorldOrigin;

        if (flag.State == FlagState::Carried)
        {
            for (std::size_t row{ 0 }; row < (std::min)(players.Rows.size(), k_MaxPlayers); ++row)
            {
                const Player& player{ players.Rows[row] };
                if (player.Handle == flag.CarrierPlayer || (flag.CarrierPlayer == k_NoHandle && player.BipedHandle == flag.CarrierBiped))
                {
                    out.CarrierRow = static_cast<std::uint8_t>(row);
                    break;
                }
            }
        }

        const auto stand{ objectives.Stands.find(flag.HomeStand) };
        if (stand != objectives.Stands.end()) out.StandPosition = stand->second.WorldOrigin;
    }

    // Fills the two flags of the View: [0] the one that the team of the agent plays in this round, [1] the rival one (D113, always).
    // While the engine recreates a flag (reset, return, capture) the row has no flag but keeps the stand of its side (D370).
    auto FillFlags(View& view, const ObjectiveDetails& objectives, const Players& players, const Entities& entities) -> void
    {
        for (std::size_t index{ 0 }; index < view.Flags.size(); ++index)
        {
            const ObjectiveDetail* flag{ FlagOfSide(objectives, view.Head.Team, index == 0) };
            if (flag)
            {
                FillFlag(view.Flags[index], *flag, objectives, players, entities);
                continue;
            }

            const StandDetail* stand{ StandOfSide(objectives, view.Head.Team, index == 0) };
            if (stand) view.Flags[index].StandPosition = stand->WorldOrigin;
        }
    }
}

namespace Views::View::System
{
    auto ViewsService::Update(const AgentSet& agents) -> void
    {
        ViewTable table{};
        const auto players{ m_PlayersStore.Acquire() };
        if (!players)
        {
            m_ViewsStore.Publish(std::move(table));
            return;
        }

        const auto entities{ m_EntitiesStore.Acquire() };
        const auto weapons{ m_WeaponDetailsStore.Acquire() };
        const auto objectives{ m_ObjectiveDetailsStore.Acquire() };
        const auto objects{ m_ObjectStore.Acquire() };
        const std::vector<std::uint32_t> locked{ LockedTargets(weapons.get()) };

        const std::size_t rows{ (std::min)(players->Rows.size(), k_MaxPlayers) };

        // The scene is prepared once and the exclusions built once, only if some agent has a body to cast from.
        const auto graph{ m_GraphStore.Acquire() };
        PlayerExclusions exclusions{};
        AimRanges ranges{};
        bool canCast{ false };
        if (entities && objects && graph)
        {
            for (std::size_t row{ 0 }; row < rows && !canCast; ++row)
            {
                canCast = agents.Contains(players->Rows[row].Handle) && players->Rows[row].Body.has_value();
            }
        }
        if (canCast)
        {
            PrepareScene(m_SceneStore.Scene(), m_SceneStore.Parts(), m_CollidableService, m_ObjectStore);
            exclusions = BuildGraphExclusions(*players, *entities, *graph);
        }

        // The eye of every living player with a biped definition: the agents need theirs, and the sight pass the ones of all the players.
        std::array<std::optional<Vec3>, k_MaxPlayers> eyes{};
        if (objects)
        {
            for (std::size_t row{ 0 }; row < rows; ++row)
            {
                const Player& player{ players->Rows[row] };
                if (canCast || agents.Contains(player.Handle)) eyes[row] = EyeOf(player, *objects, m_DefinitionsStore);
            }
        }

        for (std::size_t row{ 0 }; row < rows; ++row)
        {
            const Player& player{ players->Rows[row] };
            if (!agents.Contains(player.Handle)) continue;

            ::View& view{ table[row] };
            view.Head.PlayerHandle = player.Handle;
            view.Head.PlayerRow = static_cast<std::uint8_t>(row);
            view.Head.LocalIndex = player.LocalIndex.value_or(k_NoRow);
            view.Head.BipedHandle = player.BipedHandle;
            view.Head.Mode = agents.Mode;
            view.Head.IsComputed = true;
            view.Head.HasBody = player.IsAlive();
            view.Head.Team = player.Team;
            view.Head.Side = objectives ? SideOf(*objectives, player.Team) : Team::Neutral;

            ViewSelf& self{ view.Self };
            self.RespawnTicks = player.RespawnTicks;
            self.Powerup = player.Powerup;
            self.PowerupTicks = player.PowerupTicks;

            // Without a body the handles of the body do not exist and there is no zoom, as in the bridge.
            self.Zoom = ZoomLevel::None;
            self.Ability = k_NoHandle;
            self.TicksSinceDamage = k_NoHandle;
            for (ViewWeapon& slot : self.Weapons) slot.Handle = k_NoHandle;
            if (!player.Body) continue;

            const Body& body{ *player.Body };
            self.FrameYaw = std::atan2(body.Aim.Y, body.Aim.X);
            self.AimPitch = std::atan2(body.Aim.Z, std::hypot(body.Aim.X, body.Aim.Y));
            self.OwnVelocity = body.OwnVelocity;
            self.Shield = ValueOrNaN(body.Shield);
            self.Ground = body.Ground;
            self.IsGrounded = body.IsGrounded;
            self.AirborneTicks = body.AirborneTicks;
            self.CrouchFraction = body.CrouchFraction;
            self.Zoom = body.Zoom;
            self.TicksAlive = body.TicksAlive;
            self.SelectedGrenade = body.Loadout.SelectedGrenade;
            self.FragGrenades = body.Loadout.FragGrenades;
            self.PlasmaGrenades = body.Loadout.PlasmaGrenades;
            self.Ability = body.Loadout.Ability;
            self.AbilityEnergy = body.Loadout.AbilityEnergy;
            self.IsAbilityActive = body.Loadout.IsAbilityActive;
            self.CarriesFlag = body.Loadout.Objective != k_NoHandle;
            self.TicksSinceDamage = body.TicksSinceDamage.value_or(k_NoHandle);

            const std::array<std::uint32_t, k_ViewWeaponSlots> slots{ body.Loadout.PrimaryWeapon, body.Loadout.SecondaryWeapon };
            for (std::size_t slot{ 0 }; slot < slots.size(); ++slot)
            {
                FillWeapon(self.Weapons[slot], slots[slot], body.Loadout.EquippedWeapon, entities.get(), weapons.get());
            }

            const Entity* biped{ entities ? entities->Find(player.BipedHandle) : nullptr };
            self.Vitality = ValueOrNaN(std::nullopt);
            if (biped)
            {
                self.Feet = biped->WorldOrigin;
                self.LinearVelocity = biped->LinearVelocity;
                self.Vitality = ValueOrNaN(biped->Vitality);
                self.IsTracked = IsTrackedBy(locked, *biped);
            }

            if (!objects) continue;

            const std::optional<Vec3>& eye{ eyes[row] };
            if (eye) self.Eye = *eye;

            if (!canCast) continue;
            const RayContext context{ m_Raycaster, m_SceneStore.Scene(), exclusions, *entities, *objects, *players, m_DefinitionsStore, ranges };
            CastViewRays(view, player, body.Aim, eye.has_value(), row, context);
        }

        // The sight pass: one for the whole Update, after the rays of every agent, over the scene they prepared (D310).
        SightSeen seen{};
        m_SceneStore.Scene().SightCandidates = 0;
        m_SceneStore.Scene().SightRays = 0;
        if (canCast)
        {
            SightPlayers sightPlayers{};
            for (std::size_t row{ 0 }; row < rows; ++row)
            {
                const Player& player{ players->Rows[row] };
                if (!eyes[row]) continue;

                sightPlayers[row] = SightPlayer{ .HasHead = true, .Head = *eyes[row], .Aim = player.Body->Aim,
                    .IsViewer = agents.Contains(player.Handle) && agents.Mode == ViewMode::HumanLike };
            }

            SightCounters counters{};
            seen = ComputeSight(m_Raycaster, m_SceneStore.Scene(), exclusions, sightPlayers, counters);
            m_SceneStore.Scene().SightCandidates = counters.Candidates;
            m_SceneStore.Scene().SightRays = counters.Rays;
        }

        // What each agent knows of the other players and of the flags.
        if (entities)
        {
            for (std::size_t row{ 0 }; row < rows; ++row)
            {
                if (!agents.Contains(players->Rows[row].Handle)) continue;

                FillOthers(table[row], *players, *entities, seen);
                if (objectives) FillFlags(table[row], *objectives, *players, *entities);
            }
        }

        m_ViewsStore.Publish(std::move(table));
    }

    auto ViewsService::Cleanup() -> void
    {
        m_ViewsStore.Cleanup();
        m_SceneStore.Cleanup();

        m_Log.Message("[ViewsService] INFO: Cleanup completed.");
    }

    auto FindObservedView(const ViewTable& views, std::uint32_t observedHandle) -> const ::View*
    {
        const ::View* local{ nullptr };
        for (const ::View& view : views)
        {
            if (!view.Head.IsComputed) continue;
            if (observedHandle != k_NoHandle && view.Head.PlayerHandle == observedHandle) return &view;
            if (view.Head.LocalIndex == k_PrimaryLocalUser && !local) local = &view;
        }

        return local;
    }

    auto ObservedPlayerOf(const Perception::Players::Type::Players& players, std::uint32_t selected) -> std::uint32_t
    {
        if (selected == k_NoHandle) return k_NoHandle;

        for (const auto& player : players.Rows)
        {
            if (player.BipedHandle == selected) return player.Handle;
        }

        return k_NoHandle;
    }

    auto AimDirectionOf(const ::View& view) -> Vec3
    {
        const float aimCos{ std::cos(view.Self.AimPitch) };
        return Vec3{ aimCos * std::cos(view.Self.FrameYaw), aimCos * std::sin(view.Self.FrameYaw), std::sin(view.Self.AimPitch) };
    }

    auto ViewPositionOf(const ::View& view) -> Vec3
    {
        const Vec3& eye{ view.Self.Eye };
        return (eye.X == 0.0f && eye.Y == 0.0f && eye.Z == 0.0f) ? view.Self.Feet : eye;
    }

    auto RaycastsOf(const ::View& view) -> Raycasts
    {
        Raycasts result{};

        const ViewAimHit& aim{ view.AimHit };
        RaycastHit& aimHit{ result.AimHit };
        aimHit.Origin = view.Self.Eye;
        aimHit.Direction = AimDirectionOf(view);
        aimHit.MaxDistance = aim.MaxDistance;
        aimHit.Kind = aim.Kind;
        aimHit.Hit = aim.Kind != HitKind::None;
        aimHit.SurfaceFlags = aim.SurfaceFlags;
        aimHit.Material = aim.Material;
        if (aimHit.Hit)
        {
            aimHit.Distance = aim.Distance;
            aimHit.Point = aim.Point;
            aimHit.RegionIndex = aim.RegionIndex;
            if (aim.Kind == HitKind::Dynamic) aimHit.ObjectHandle = aim.ObjectHandle;
        }

        // The rays were cast only if the first one has a distance (a miss reports its range).
        if (!view.Head.HasBody || view.Rays[0].Distance <= 0.0f) return result;

        result.PerceptionHits.reserve(k_PatternRays.size());
        for (std::size_t index{ 0 }; index < k_PatternRays.size(); ++index)
        {
            const PatternRay& ray{ k_PatternRays[index] };
            const ViewRayHit& source{ view.Rays[index] };
            RaycastHit hit{};
            hit.Origin = view.RayOrigin;
            hit.Direction = PatternDirectionAt(ray, view.Self.FrameYaw);
            hit.MaxDistance = ray.Range;
            hit.Kind = source.Kind;
            hit.Hit = source.Kind != HitKind::None;
            hit.SurfaceFlags = source.SurfaceFlags;
            hit.Material = source.Material;
            if (hit.Hit)
            {
                hit.Distance = source.Distance;
                hit.Point = Vec3{ hit.Origin.X + hit.Direction.X * source.Distance, hit.Origin.Y + hit.Direction.Y * source.Distance,
                    hit.Origin.Z + hit.Direction.Z * source.Distance };
                if (source.Kind == HitKind::Dynamic) hit.ObjectHandle = source.ObjectHandle;
            }
            result.PerceptionHits.push_back(hit);
        }

        return result;
    }
}