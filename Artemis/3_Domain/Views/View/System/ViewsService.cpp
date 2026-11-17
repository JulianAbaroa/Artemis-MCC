module Views.View.System;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Details.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
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
    using Perception::Players::Type::Body;
    using Perception::Players::Type::Player;
    using Views::Raycast::System::DerivedEye;
    using Views::View::Type::View;
    using Views::View::Type::ViewMode;
    using Views::View::Type::ViewSelf;
    using Views::View::Type::ViewTable;
    using Views::View::Type::ViewWeapon;
    using Views::View::Type::k_ViewWeaponSlots;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;

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
        }

        m_ViewsStore.Publish(std::move(table));
    }

    auto ViewsService::Cleanup() -> void
    {
        m_ViewsStore.Cleanup();
    }
}