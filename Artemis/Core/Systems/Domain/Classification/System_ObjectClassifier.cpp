#include "pch.h"

// Header.
#include "System_ObjectClassifier.h"

// --- States ---

#include "Core/States/Domain/Object/State_ObjectTable.h"
#include "Core/States/Domain/Player/State_PlayerTable.h"
#include "Core/States/Domain/Classification/State_Classification.h"
#include "Core/States/Domain/Navigation/State_Navigation.h"
#include "Core/States/Domain/Environment/State_Environment.h"
#include "Core/States/Domain/Interactable/State_Interactable.h"

// --- Systems ---

#include "Core/Systems/Interface/Debug/System_Debug.h"

#include <unordered_set>

void System_ObjectClassifier::UpdateClassification()
{
    uint32_t selfPlayerHandle = 
        m_Deps.State_PlayerTable.GetPlayerHandleByName("Artemis11010");

    const LivePlayer* selfPtr = 
        m_Deps.State_PlayerTable.GetPlayer(selfPlayerHandle);

    // Obtain the self, current biped handle.
    uint32_t selfBipedHandle = 0xFFFFFFFF;
    if (selfPtr) selfBipedHandle = selfPtr->CurrentBipedHandle;

    const auto& objectTable = m_Deps.State_ObjectTable.GetObjectTable();

    std::vector<ClassifiedObject> results;
    results.reserve(objectTable.size());

    for (const auto& [handle, object] : objectTable)
    {
        if (object.Address == 0) continue;

        ObjectRole role = this->ClassifyNode(
            handle, selfBipedHandle, object, objectTable);
        if (role == ObjectRole::Unknown) continue;

        ClassifiedObject object{};
        object.Handle = handle;
        object.Role = role;

        results.push_back(std::move(object));
    }

    m_Deps.State_Classification.SetObjects(std::move(results));
}

void System_ObjectClassifier::Cleanup()
{
    m_Deps.State_Classification.Cleanup();
    m_Deps.System_Debug.Log("[System_ObjectClassifier] INFO: Cleanup completed.");
}

ObjectRole System_ObjectClassifier::ClassifyNode(uint32_t handle, 
    uint32_t selfBipedHandle, const LiveObject& object,
    const std::unordered_map<uint32_t, LiveObject>& nodes) const
{
    switch (object.Profile.Class)
    {
    case ObjectClass::Biped:
        return this->ClassifyBiped(handle, selfBipedHandle);

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

ObjectRole System_ObjectClassifier::ClassifyBiped(uint32_t handle,
    uint32_t selfBipedHandle) const
{
    if (handle == selfBipedHandle)
    {
        return ObjectRole::SelfBiped;
    }

    return ObjectRole::Biped;
}

ObjectRole System_ObjectClassifier::ClassifyVehicle(const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    // Vehicles
    if (TagName::Vehicle::IsVehicle(tag))
    {
        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return ObjectRole::VehiclePart;
        }
        
        return ObjectRole::Vehicle;
    }

    // Special-case, mounted machineguns are vehicles for the game.
    if (TagName::Weapon::IsVehicle(tag)) return ObjectRole::Vehicle;

    return ObjectRole::Unknown;
}

ObjectRole System_ObjectClassifier::ClassifyWeapon(const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    using namespace TagName;

    // Objective.
    if (Objective::IsObjective(tag))
    {
        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return ObjectRole::ObjectiveEquiped;
        }
        
        return ObjectRole::ObjectivePickup;
    }

    // Weapon.
    if (Weapon::IsWeapon(tag))
    {
        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return ObjectRole::WeaponEquiped;
        }
        
        ObjectRole::WeaponPickup;
    }

    return ObjectRole::Unknown;
}

ObjectRole System_ObjectClassifier::ClassifyEquipment(const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    // If is weapon ammo. (default, rockets, sniper)
    if (TagName::Ammo::IsAmmo(tag)) return ObjectRole::AmmoPickup;

    if (TagName::ArmorAbility::IsArmorAbility(tag))
    {
        if (object.ParentHandle != 0xFFFFFFFF)
        {
            return ObjectRole::ArmorAbilityEquiped;
        }
        
        return ObjectRole::ArmorAbilityPickup;
    }

    if (TagName::Weapon::IsGrenade(tag))
    {
        return ObjectRole::GrenadePickup;
    }

    return ObjectRole::Unknown;
}

ObjectRole System_ObjectClassifier::ClassifyScenery(const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    // Spawn.
    if (TagName::Spawn::IsSpawn(tag))
    {
        return ObjectRole::Spawn;
    }

    // Boundaries.
    if (TagName::Boundary::IsBoundary(tag))
    {
        return ObjectRole::Boundary;
    }

    // Portable Shield.
    if (TagName::Shield::IsPortableShield(tag))
    {
        return ObjectRole::PortableShield;
    }

    // None.
    if (TagName::SkyBox::IsSkyBox(tag))
    {
        return ObjectRole::None;
    }

    // Obstacles.
    const SceneryObstacleData* obstacle = 
        m_Deps.State_Navigation.GetScenObstacle(tag);
    // TODO: This works fine?
    if (obstacle && !obstacle->NotAPathfindingObstacle &&
        obstacle->PathfindingPolicy != ScenPathfindingPolicy::None)
    {
        return ObjectRole::SceneryObstacle;
    }

    return ObjectRole::Unknown;
}

ObjectRole System_ObjectClassifier::ClassifyBloc(const LiveObject& object) const
{
    const std::string& tag = object.TagName;

    // Objective.
    if (TagName::Objective::IsObjectiveSpawn(tag))
    {
        return ObjectRole::ObjectiveSpawn;
    }

    // Teleport.
    if (TagName::Teleport::IsTeleport(tag))
    {
        return ObjectRole::Teleporter;
    }

    // Explosive.
    if (TagName::Explosive::IsExplosive(tag))
    {
        return ObjectRole::Explosive;
    }

    // Pallet.
    if (TagName::Palletes::IsPallete(tag))
    {
        return ObjectRole::Pallet;
    }

    // Lift.
    if (TagName::Lift::IsLift(tag))
    {
        return ObjectRole::Lift;
    }

    // Shield.
    if (TagName::Shield::IsShield(tag))
    {
        return ObjectRole::Shield;
    }

    const CrateObstacleData* obstacle =
        m_Deps.State_Navigation.GetBlocObstacle(object.TagName);
    // TODO: This works fine?
    if (obstacle && !obstacle->NotAPathfindingObstacle)
        return ObjectRole::CrateObstacle;

    return ObjectRole::Unknown;
}

ObjectRole System_ObjectClassifier::ClassifyDeviceControl(
    const LiveObject& object) const
{
    const ControlDeviceData* ctrl =
        m_Deps.State_Interactable.GetControlDeviceData(object.TagName);

    if (ctrl && ctrl->DeviceType == CtrlDeviceType::Health)
    {
        return ObjectRole::HealthStation;
    }

    return ObjectRole::Unknown;
}