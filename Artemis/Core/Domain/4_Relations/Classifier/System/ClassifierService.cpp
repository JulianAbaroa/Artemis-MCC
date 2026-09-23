module Relations.Classifier.System;

import Common.Tag.Name;

namespace
{
    namespace TagName = Common::Tag::Name;

    using Class = Tables::Object::Type::Class::Class;
    using Classified = Relations::Classifier::Type::Classified;
    using Classifieds = std::vector<Classified>;
}

namespace Relations::Classifier::System
{
    auto ClassifierService::UpdateClassification() -> void
    {
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!objectTablePtr) return;
        const ObjectTable& objectTable = *objectTablePtr;

        Classifieds results;
        results.reserve(objectTable.size());

        for (const auto& [handle, object] : objectTable)
        {
            if (object.Address == 0) continue;

            Role role = this->ClassifyNode(
                handle, object, objectTable);

            if (role == Role::Unknown) continue;

            ::Classified classified{};
            classified.Handle = handle;
            classified.Role = role;

            results.push_back(std::move(classified));
        }

        m_ClassifierStore.Publish(std::move(results));
    }

    auto ClassifierService::ClassifyNode(uint32_t handle, const AliveObject& object,
        const ObjectTable& objectTable) const -> Role
    {
        switch (object.Profile.Class)
        {
        case Class::Biped:
            return Role::Biped;

        case Class::Vehicle:
            return this->ClassifyVehicle(object);

        case Class::Weapon:
            return this->ClassifyWeapon(object);

        case Class::Equipment:
            return this->ClassifyEquipment(object);

        case Class::Projectile:
            return Role::Projectile;

        case Class::Scenery:
            return this->ClassifyScenery(object);

        case Class::DeviceMachine:
            return Role::DeviceMachine;

        case Class::DeviceControl:
            return this->ClassifyDeviceControl(object);

        case Class::Crate:
            return this->ClassifyBloc(object);

        default:
            return Role::Unknown;
        }
    }

    auto ClassifierService::ClassifyVehicle(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        if (TagName::Vehicle::IsVehicle(tag))
        {
            if (object.ParentHandle != 0xFFFFFFFF)
            {
                return Role::VehiclePart;
            }

            return Role::Vehicle;
        }

        if (TagName::Weapon::IsVehicle(tag))
        {
            return Role::Vehicle;
        }

        return Role::Unknown;
    }

    auto ClassifierService::ClassifyWeapon(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        using namespace TagName;

        if (Objective::IsObjective(tag))
        {
            if (object.ParentHandle != 0xFFFFFFFF)
            {
                return Role::ObjectiveEquipped;
            }

            return Role::ObjectivePickup;
        }

        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return Role::WeaponEquipped;
        }

        return Role::WeaponPickup;
    }

    auto ClassifierService::ClassifyEquipment(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        if (TagName::Ammo::IsAmmo(tag))
        {
            return Role::AmmoPickup;
        }

        if (TagName::ArmorAbility::IsArmorAbility(tag))
        {
            if (object.ParentHandle != 0xFFFFFFFF)
            {
                return Role::ArmorAbilityEquipped;
            }

            return Role::ArmorAbilityPickup;
        }

        if (TagName::Weapon::IsGrenade(tag))
        {
            return Role::GrenadePickup;
        }

        if (TagName::Powerup::IsPowerup(tag))
        {
            return Role::Powerup;
        }

        return Role::Unknown;
    }

    auto ClassifierService::ClassifyScenery(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        if (TagName::Spawn::IsSpawn(tag))
        {
            return Role::Spawn;
        }

        if (TagName::Boundary::IsBoundary(tag))
        {
            return Role::Boundary;
        }

        if (TagName::Shield::IsPortableShield(tag))
        {
            return Role::PortableShield;
        }

        if (TagName::SkyBox::IsSkyBox(tag))
        {
            return Role::None;
        }

        // TODO: More filters needs to be applied here.
        return Role::SceneryObstacle;
    }

    auto ClassifierService::ClassifyBloc(const AliveObject& object) const -> Role
    {
        const std::string& tag = object.TagName;

        if (TagName::Objective::IsObjectiveSpawn(tag))
        {
            return Role::ObjectiveSpawn;
        }

        // TODO: Add objective zone to other systems.
        if (TagName::Objective::IsObjectiveZone(tag))
        {
            return Role::ObjectiveZone;
        }

        if (TagName::Teleport::IsTeleport(tag))
        {
            return Role::Teleporter;
        }

        if (TagName::Explosive::IsExplosive(tag))
        {
            return Role::Explosive;
        }

        if (TagName::Palletes::IsPallete(tag))
        {
            return Role::Pallet;
        }

        if (TagName::Lift::IsLift(tag))
        {
            return Role::Lift;
        }

        if (TagName::Shield::IsShield(tag))
        {
            return Role::Shield;
        }

        // TODO: More filters needs to be applied here.
        return Role::CrateObstacle;
    }

    auto ClassifierService::ClassifyDeviceControl(const AliveObject& object) const -> Role
    {
        if (TagName::ControlDevice::IsHealthStation(object.TagName))
        {
            return Role::HealthStation;
        }

        return Role::Unknown;
    }

    auto ClassifierService::Cleanup() -> void
    {
        m_ClassifierStore.Cleanup();

        m_LogsService.Message("[ClassifierService] INFO: Cleanup completed.");
    }
}