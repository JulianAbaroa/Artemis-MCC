#include "pch.h"

#include "System_Classifier.h"

#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Sources/Tables/Player/State_PlayerTable.h"
#include "Core/States/Structure/Classifier/State_Classifier.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <unordered_set>

void System_Classifier::UpdateClassification()
{
    auto objectTablePtr = m_Deps.State_ObjectTable.Acquire();
    if (!objectTablePtr) return;
    const ObjectTable& objectTable = *objectTablePtr;

    Classifieds results;
    results.reserve(objectTable.size());

    for (const auto& [handle, object] : objectTable)
    {
        if (object.Address == 0) continue;

        ObjectRole role = this->ClassifyNode(
            handle, object, objectTable);

        if (role == ObjectRole::Unknown) continue;

        Classified classified{};
        classified.Handle = handle;
        classified.Role = role;

        results.push_back(std::move(classified));
    }

    m_Deps.State_Classifier.Publish(std::move(results));
}

ObjectRole System_Classifier::ClassifyNode(uint32_t handle, 
    const LiveObject& object, const ObjectTable& objectTable) const
{
    switch (object.Profile.Class)
    {
    case ObjectClass::Biped:
        return ObjectRole::Biped;

    case ObjectClass::Vehicle:
        return this->ClassifyVehicle(object);

    case ObjectClass::Weapon:
        return this->ClassifyWeapon(object);

    case ObjectClass::Equipment:
        return this->ClassifyEquipment(object);

    case ObjectClass::Projectile:
        return ObjectRole::Projectile;

    case ObjectClass::Scenery:
        return this->ClassifyScenery(object);

    case ObjectClass::DeviceMachine:
        return ObjectRole::DeviceMachine;

    case ObjectClass::DeviceControl:
        return this->ClassifyDeviceControl(object);

    case ObjectClass::Crate:
        return this->ClassifyBloc(object);

    default:
        return ObjectRole::Unknown;
    }
}

ObjectRole System_Classifier::ClassifyVehicle(
    const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    if (TagName::Vehicle::IsVehicle(tag))
    {
        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return ObjectRole::VehiclePart;
        }
        
        return ObjectRole::Vehicle;
    }

    if (TagName::Weapon::IsVehicle(tag))
    {
        return ObjectRole::Vehicle;
    }

    return ObjectRole::Unknown;
}

ObjectRole System_Classifier::ClassifyWeapon(
    const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    using namespace TagName;

    if (Objective::IsObjective(tag))
    {
        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return ObjectRole::ObjectiveEquipped;
        }
        
        return ObjectRole::ObjectivePickup;
    }

    if (object.ParentHandle != 0xFFFFFFFF)
    {
        return ObjectRole::WeaponEquipped;
    }
    
    return ObjectRole::WeaponPickup;
}

ObjectRole System_Classifier::ClassifyEquipment(
    const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    if (TagName::Ammo::IsAmmo(tag))
    {
        return ObjectRole::AmmoPickup;
    }

    if (TagName::ArmorAbility::IsArmorAbility(tag))
    {
        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return ObjectRole::ArmorAbilityEquipped;
        }
        
        return ObjectRole::ArmorAbilityPickup;
    }

    if (TagName::Weapon::IsGrenade(tag))
    {
        return ObjectRole::GrenadePickup;
    }

    if (TagName::Powerup::IsPowerup(tag))
    {
        return ObjectRole::Powerup;
    }

    return ObjectRole::Unknown;
}

ObjectRole System_Classifier::ClassifyScenery(
    const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    if (TagName::Spawn::IsSpawn(tag))
    {
        return ObjectRole::Spawn;
    }

    if (TagName::Boundary::IsBoundary(tag))
    {
        return ObjectRole::Boundary;
    }

    if (TagName::Shield::IsPortableShield(tag))
    {
        return ObjectRole::PortableShield;
    }

    if (TagName::SkyBox::IsSkyBox(tag))
    {
        return ObjectRole::None;
    }

    // TODO: More filters needs to be applied here.
    return ObjectRole::SceneryObstacle;
}

ObjectRole System_Classifier::ClassifyBloc(
    const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    if (TagName::Objective::IsObjectiveSpawn(tag))
    {
        return ObjectRole::ObjectiveSpawn;
    }

    // TODO: Add objective zone to other systems.
    if (TagName::Objective::IsObjectiveZone(tag))
    {
        return ObjectRole::ObjectiveZone;
    }

    if (TagName::Teleport::IsTeleport(tag))
    {
        return ObjectRole::Teleporter;
    }

    if (TagName::Explosive::IsExplosive(tag))
    {
        return ObjectRole::Explosive;
    }

    if (TagName::Palletes::IsPallete(tag))
    {
        return ObjectRole::Pallet;
    }

    if (TagName::Lift::IsLift(tag))
    {
        return ObjectRole::Lift;
    }

    if (TagName::Shield::IsShield(tag))
    {
        return ObjectRole::Shield;
    }

    // TODO: More filters needs to be applied here.
    return ObjectRole::CrateObstacle;
}

ObjectRole System_Classifier::ClassifyDeviceControl(
    const LiveObject& object) const
{
    if (TagName::ControlDevice::IsHealthStation(object.TagName))
    {
        return ObjectRole::HealthStation;
    }

    return ObjectRole::Unknown;
}

void System_Classifier::Cleanup()
{
    m_Deps.State_Classifier.Cleanup();

    m_Deps.System_Logs.Log("[Classifier] INFO:"
        " Cleanup completed.");
}