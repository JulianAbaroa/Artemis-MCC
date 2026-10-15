module Environment.Players.System;

import Common.Semantic.Type;

namespace
{
    using Common::Semantic::Type::Lifecycle;
    using Tables::Object::Type::Biped::Biped;
    using Tables::Object::Type::Biped::Damage;
    using Tables::Object::Type::Equipment::Equipment;
    using Environment::Details::Type::VitalityDetails;

    // Value of an empty handle.
    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };

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

    // Returns the player of the latest hit, or k_NoHandle without attackers.
    // note: The victim itself after its own grenade or a fall.
    auto FindLastDamager(const std::vector<Damage>& damages) -> std::uint32_t
    {
        const auto latest{ std::ranges::max_element(damages, {}, &Damage::Tick) };
        return latest == damages.end() ? k_NoHandle : latest->PlayerHandle;
    }
}

namespace Environment::Players::System
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

        Players players{};
        players.Rows.reserve(playerTablePtr->size());

        for (const auto& [handle, alive] : *playerTablePtr)
        {
            players.Rows.push_back(this->MakePlayer(alive, *objectTablePtr, *entitiesPtr, vitalities));
        }

        std::ranges::sort(players.Rows, {}, &Player::Handle);

        players.Index.reserve(players.Rows.size());
        for (std::size_t row{ 0 }; row < players.Rows.size(); ++row)
        {
            players.Index.emplace(players.Rows[row].Handle, row);
        }

        m_PlayersStore.Publish(std::move(players));
    }

    auto PlayersService::MakePlayer(const AlivePlayer& alive, const ObjectTable& objectTable, const Entities& entities,
        const VitalityDetails& vitalities) const -> Player
    {
        Player player{};
        player.Handle = alive.Handle;
        player.Name = alive.Gamertag.empty() ? alive.Tag : alive.Gamertag;
        player.Connection = alive.ConnectionState;
        player.Team = alive.Team;
        player.CameraPosition = alive.CameraPosition;
        player.CameraForward = alive.CameraForward;
        player.Body = this->MakeBody(alive, objectTable, entities, vitalities);
        player.BipedHandle = player.Body ? alive.AliveBipedHandle : alive.CurrentBipedHandle;

        return player;
    }

    auto PlayersService::MakeBody(const AlivePlayer& alive, const ObjectTable& objectTable, const Entities& entities,
        const VitalityDetails& vitalities) const -> std::optional<Body>
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
        m_PlayersStore.Cleanup();

        m_LogsService.Message("[PlayersService] INFO: Cleanup completed.");
    }
}