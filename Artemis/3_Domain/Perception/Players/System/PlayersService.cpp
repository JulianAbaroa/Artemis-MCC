module Perception.Players.System;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Blam.Layout.Player.Type;

namespace
{
    using Common::Semantic::Type::Lifecycle;
    using Blam::Layout::Player::Type::Powerup::ActivePowerup;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Blam::Runtime::Object::Type::Biped::Biped;
    using Blam::Runtime::Object::Type::Biped::Damage;
    using Common::Math::Type::Vec3;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::GroundKind;
    using Perception::Players::Type::GroundMemoryEntry;
    using Blam::Runtime::Object::Type::Equipment::Equipment;
    using Perception::Details::Type::VitalityDetails;

    using Blam::Runtime::Object::Type::Biped::k_NoContact;
    using Common::Engine::Type::k_NoHandle;

    // Ticks without contact after which the biped is no longer grounded.
    constexpr std::uint32_t k_HoldTicks{ 3 };

    // Own vertical speed above which the first tick without contact is a jump, in world units per second.
    constexpr float k_TakeoffSpeed{ 1.0f };

    // CrouchFraction from which the biped counts as crouching.
    constexpr float k_CrouchThreshold{ 0.5f };

    // Returns the vitality of the first shield section of an object, from 0 to 1.
    // return: Empty when the object has no vitality detail or its layout has no shield section.
    auto FindShield(const VitalityDetails& vitalities, std::uint32_t handle) -> std::optional<float>
    {
        const auto it{ vitalities.find(handle) };
        if (it == vitalities.end() || !it->second.Layout) return std::nullopt;

        const int section{ it->second.Layout->ShieldSection };
        if (section < 0 || static_cast<std::size_t>(section) >= it->second.Sections.size()) return std::nullopt;

        return it->second.Sections[static_cast<std::size_t>(section)];
    }

    // Returns the velocity of the biped relative to the object under its feet.
    // return: The velocity of the biped itself when the ground is not an object or the object is not in the table.
    auto FindOwnVelocity(const AliveObject& object, GroundKind ground, std::uint32_t groundObject, const ObjectTable& objectTable) -> Vec3
    {
        Vec3 velocity{ object.LinearVelocity };
        if (ground != GroundKind::Object) return velocity;

        const auto it{ objectTable.find(groundObject) };
        if (it == objectTable.end()) return velocity;

        velocity.X -= it->second.LinearVelocity.X;
        velocity.Y -= it->second.LinearVelocity.Y;
        velocity.Z -= it->second.LinearVelocity.Z;
        return velocity;
    }

    // Resolves the ground of a biped from its contact and the memory of the previous tick (D260).
    // param next: Receives the memory for the next tick.
    // note: The contact cuts for 1 or 2 ticks between blocks, so the last ground is held for them, unless the biped jumped off it.
    auto ResolveGround(Body& body, const AliveObject& object, const Biped& biped, const ObjectTable& objectTable,
        const GroundMemoryEntry& previous, GroundMemoryEntry& next) -> void
    {
        const bool isAttached{ object.ParentHandle != k_NoHandle };
        const bool isContact{ !isAttached && biped.ContactMaterial != k_NoContact };

        next = previous;
        body.GroundObject = k_NoHandle;

        if (isAttached || isContact)
        {
            next.AirborneTicks = 0;
            next.IsTakeoff = false;

            if (isAttached)
            {
                body.Ground = GroundKind::Attached;
                next.LastObject = k_NoHandle;
            }
            else if (biped.OnStructure)
            {
                body.Ground = GroundKind::Static;
                next.LastObject = k_NoHandle;
            }
            else
            {
                body.Ground = GroundKind::Object;
                if (biped.GroundObjectHandle != k_NoHandle) next.LastObject = biped.GroundObjectHandle;
                body.GroundObject = next.LastObject;
            }

            next.LastGround = body.Ground;
            body.IsGrounded = !isAttached;
        }
        else
        {
            next.AirborneTicks = previous.AirborneTicks + 1;

            const bool wasOnGround{ previous.LastGround == GroundKind::Static || previous.LastGround == GroundKind::Object };
            if (next.AirborneTicks == 1 && wasOnGround)
            {
                const Vec3 own{ FindOwnVelocity(object, previous.LastGround, previous.LastObject, objectTable) };
                next.IsTakeoff = own.Z > k_TakeoffSpeed;
            }

            const bool isHeld{ wasOnGround && !next.IsTakeoff && next.AirborneTicks < k_HoldTicks };
            if (isHeld)
            {
                body.Ground = previous.LastGround;
                if (body.Ground == GroundKind::Object) body.GroundObject = previous.LastObject;
            }
            else
            {
                body.Ground = GroundKind::Air;
                next.LastGround = GroundKind::Air;
                next.LastObject = k_NoHandle;
            }
            body.IsGrounded = isHeld;
        }

        body.AirborneTicks = next.AirborneTicks;
        body.OwnVelocity = FindOwnVelocity(object, body.Ground, body.GroundObject, objectTable);
    }

    // Returns the player of the latest hit, or k_NoHandle without attackers.
    // note: The victim itself after its own grenade or a fall.
    auto FindLastDamager(const std::vector<Damage>& damages) -> std::uint32_t
    {
        const auto latest{ std::ranges::max_element(damages, {}, &Damage::Tick) };
        return latest == damages.end() ? k_NoHandle : latest->PlayerHandle;
    }
}

namespace Perception::Players::System
{
    auto PlayersService::Update() -> void
    {
        auto playerTablePtr = m_PlayerStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        auto entitiesPtr = m_EntitiesStore.Acquire();
        if (!playerTablePtr || !objectTablePtr || !entitiesPtr) return;

        // note: Before the first publish of the vitalities no biped has a shield.
        const auto vitalitiesPtr{ m_VitalityDetailsStore.Acquire() };
        const VitalityDetails empty{};
        const VitalityDetails& vitalities{ vitalitiesPtr ? *vitalitiesPtr : empty };

        const auto gamePtr{ m_GameStore.Acquire() };

        Players players{};
        players.Rows.reserve(playerTablePtr->size());

        // note: The memory keeps only the bipeds that have a Body in this tick.
        GroundMemory& memory{ m_GroundMemoryStore.Get() };
        GroundMemory nextMemory{};

        for (const auto& [handle, alive] : *playerTablePtr)
        {
            players.Rows.push_back(this->MakePlayer(alive, gamePtr.get(), *objectTablePtr, *entitiesPtr, vitalities, memory, nextMemory));
        }
        memory = std::move(nextMemory);

        std::ranges::sort(players.Rows, {}, &Player::Handle);

        players.Index.reserve(players.Rows.size());
        for (std::size_t row{ 0 }; row < players.Rows.size(); ++row)
        {
            players.Index.emplace(players.Rows[row].Handle, row);
        }

        m_PlayersStore.Publish(std::move(players));
    }

    auto PlayersService::MakePlayer(const AlivePlayer& alive, const AliveGame* game, const ObjectTable& objectTable, const Entities& entities,
        const VitalityDetails& vitalities, const GroundMemory& previous, GroundMemory& nextMemory) const -> Player
    {
        Player player{};
        player.Handle = alive.Handle;
        player.Name = alive.Gamertag.empty() ? alive.Tag : alive.Gamertag;
        player.Connection = alive.ConnectionState;
        player.LocalIndex = alive.LocalIndex;
        player.Team = alive.Team;
        player.RespawnTicks = alive.RespawnTicks;
        player.Traits = alive.Traits;
        if (game)
        {
            const auto powerup{ ActivePowerup(alive.PowerupPickupTicks, game->EngineTick, game->PowerupDurationTicks) };
            player.Powerup = powerup.Subtype;
            player.PowerupTicks = powerup.Ticks;
        }
        player.CameraPosition = alive.CameraPosition;
        player.CameraForward = alive.CameraForward;
        player.Body = this->MakeBody(alive, objectTable, entities, vitalities, previous, nextMemory);
        player.BipedHandle = player.Body ? alive.AliveBipedHandle : alive.CurrentBipedHandle;

        return player;
    }

    auto PlayersService::MakeBody(const AlivePlayer& alive, const ObjectTable& objectTable, const Entities& entities,
        const VitalityDetails& vitalities, const GroundMemory& previous, GroundMemory& nextMemory) const -> std::optional<Body>
    {
        const std::uint32_t handle{ alive.AliveBipedHandle };
        if (handle == k_NoHandle) return std::nullopt;

        // note: The player table can still point to a biped whose life section is empty. Entities decides.
        const auto* entity{ entities.Find(handle) };
        if (!entity || entity->Lifecycle != Lifecycle::Active) return std::nullopt;

        const auto objectIt{ objectTable.find(handle) };
        if (objectIt == objectTable.end()) return std::nullopt;

        const auto* biped{ std::get_if<Biped>(&objectIt->second.Specific) };
        if (!biped) return std::nullopt;

        Body body{};
        body.Aim = biped->Aim;
        body.Zoom = biped->ZoomLevel;
        body.TicksAlive = biped->TicksAlive;
        body.Shield = FindShield(vitalities, handle);
        body.LastDamagerPlayer = FindLastDamager(biped->Damages);
        body.TicksSinceDamage = biped->TicksSinceDamage;
        body.CrouchFraction = biped->CrouchFraction;
        body.IsCrouching = biped->CrouchFraction >= k_CrouchThreshold;

        const auto previousIt{ previous.find(handle) };
        const GroundMemoryEntry before{ previousIt == previous.end() ? GroundMemoryEntry{} : previousIt->second };
        ResolveGround(body, objectIt->second, *biped, objectTable, before, nextMemory[handle]);

        auto& loadout{ body.Loadout };
        loadout.PrimaryWeapon = biped->PrimaryWeaponHandle;
        loadout.SecondaryWeapon = biped->SecondaryWeaponHandle;
        loadout.EquippedWeapon = biped->EquippedWeaponHandle;
        loadout.Ability = biped->ArmorAbilityHandle;
        loadout.SelectedGrenade = biped->SelectedGrenade;
        loadout.FragGrenades = biped->FragGrenades;
        loadout.PlasmaGrenades = biped->PlasmaGrenades;
        loadout.Objective = alive.ObjectiveHandle;

        const auto abilityIt{ objectTable.find(loadout.Ability) };
        if (abilityIt != objectTable.end())
        {
            if (const auto* equipment{ std::get_if<Equipment>(&abilityIt->second.Specific) })
            {
                loadout.AbilityEnergy = equipment->Energy;
                loadout.IsAbilityActive = equipment->IsActive;
            }
        }

        return body;
    }

    auto PlayersService::Cleanup() -> void
    {
        m_GroundMemoryStore.Cleanup();
        m_PlayersStore.Cleanup();

        m_Log.Message("[PlayersService] INFO: Cleanup completed.");
    }
}