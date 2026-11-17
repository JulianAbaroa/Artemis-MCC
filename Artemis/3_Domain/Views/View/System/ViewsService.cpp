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
    using Common::Team::Type::Team;
    using Blam::MapModel::Definitions::Type::Bipd::Bipd;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Blam::Runtime::Object::Type::Biped::ZoomLevel;
    using Perception::Details::Type::ObjectiveDetails;
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
    using Views::Raycast::Type::Pattern::PatternRay;
    using Views::Raycast::Type::Pattern::k_PatternRays;
    using Views::Raycast::Type::Constant::k_FallbackAimRange;
    using Views::Raycast::Type::Constant::k_MaxRayRange;
    using Views::View::Type::ViewAimHit;
    using Views::View::Type::ViewRayHit;
    using Views::View::Type::ViewRelation;
    using Perception::Players::Type::Player;
    using Views::Raycast::System::BuildGraphExclusions;
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
        out.Distance = hit.Hit ? hit.Distance : hit.MaxDistance;
        out.Kind = hit.Kind;
        out.SurfaceFlags = hit.SurfaceFlags;
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
        out.SurfaceFlags = hit.SurfaceFlags;
        if (!hit.Hit || hit.Kind != HitKind::Dynamic) return;

        const Entity* entity{ context.EntityRows.Find(hit.ObjectHandle) };
        if (entity) out.Function = entity->Function;
        out.Relation = RelationOf(team, entity);
    }

    // Casts the aim ray and the rays of the pattern of one agent through the shared scene, with the exclusion of its row.
    // note: The aim ray starts at the eye (Eye is zero when the biped has no tag: then it does not fire). The perception rays start at the
    // bounding center of the vehicle the biped rides, or of the biped, and their frame is the forward of that object, as the Self of before.
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
        Vec3 forward{}, right{}, up{};
        Common::Math::System::BuildFrame(object->second.Forward, forward, right, up);
        const Vec3 left{ -right.X, -right.Y, -right.Z };

        float radius{ 0.0f };
        for (const PatternRay& ray : k_PatternRays) radius = (std::max)(radius, ray.Range);
        const SceneReach reach{ GatherSceneReach(context.Scene, exclusion, origin, radius, nullptr) };

        for (std::size_t index{ 0 }; index < k_PatternRays.size(); ++index)
        {
            const PatternRay& ray{ k_PatternRays[index] };
            const Vec3 direction{ PatternDirection(ray, forward, left, up) };
            FillRayHit(view.Rays[index], CastSceneRayReach(context.Caster, context.Scene, reach, exclusion, origin, direction, ray.Range, nullptr), context, player.Team);
        }
    }

    // return: The value, or NaN when it is absent (Shield, Vitality), as the bridge writes it.
    auto ValueOrNaN(const std::optional<float>& value) -> float
    {
        return value.value_or(std::numeric_limits<float>::quiet_NaN());
    }

    // return: The BaseTeam of the stand the team owns in this round, Neutral when it owns none.
    // note: With more than one stand of the team the one of the lowest handle gives it, so the answer does not depend on the map order.
    auto SideOf(const ObjectiveDetails& objectives, Team team) -> Team
    {
        Team side{ Team::Neutral };
        std::uint32_t best{ k_NoHandle };
        for (const auto& [handle, stand] : objectives.Stands)
        {
            if (stand.OwnerTeam != team || handle >= best) continue;
            best = handle;
            side = stand.BaseTeam;
        }
        return side;
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
        for (std::size_t row{ 0 }; row < rows; ++row)
        {
            const Player& player{ players->Rows[row] };
            if (!agents.Contains(player.Handle)) continue;

            ::View& view{ table[row] };
            view.Head.PlayerHandle = player.Handle;
            view.Head.PlayerRow = static_cast<std::uint8_t>(row);
            view.Head.Mode = ViewMode::HumanLike;
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
            const auto object{ objects->find(player.BipedHandle) };
            if (object == objects->end()) continue;

            const Bipd* definition{ m_DefinitionsStore.m_Bipd.Get(object->second.TagName) };
            if (definition) self.Eye = DerivedEye(object->second, *definition, body.CrouchFraction);

            if (!canCast) continue;
            const RayContext context{ m_Raycaster, m_SceneStore.Scene(), exclusions, *entities, *objects, *players, m_DefinitionsStore, ranges };
            CastViewRays(view, player, body.Aim, definition != nullptr, row, context);
        }

        m_ViewsStore.Publish(std::move(table));
    }

    auto ViewsService::Cleanup() -> void
    {
        m_ViewsStore.Cleanup();
    }
}