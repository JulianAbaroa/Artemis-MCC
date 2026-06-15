#include "pch.h"

#include "System_Fixtures.h"

#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Structure/Classifier/State_Classifier.h"
#include "Core/States/Environment/Fixtures/State_Fixtures.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

void System_Fixtures::Update()
{
    auto classifiedsPtr = m_Deps.State_Classifier.Acquire();
    auto objectTablePtr = m_Deps.State_ObjectTable.Acquire();
    if (!classifiedsPtr || !objectTablePtr) return;

    const Classifieds& classifieds = *classifiedsPtr;
    const ObjectTable& objectTable = *objectTablePtr;

    Fixtures data;
    this->CollectObstacles(classifieds, objectTable, data.Obstacles);
    this->CollectSpawns(classifieds, objectTable, data.Spawns);
    this->CollectTeleports(classifieds, objectTable, data.Teleporters);
    this->CollectLifts(classifieds, objectTable, data.Lifts);
    this->CollectShields(classifieds, objectTable, data.Shields);
    this->CollectObjectives(classifieds, objectTable, data.Objectives, data.ObjectiveSpawns);
    this->CollectDestructibles(classifieds, objectTable, data.Destructibles);

    m_Deps.State_Fixtures.Publish(std::move(data));
}

void System_Fixtures::CollectObstacles(const Classifieds& classifieds,
    const ObjectTable& objects, Obstacles& obstacles)
{
    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::CrateObstacle ||
            classified.Role != ObjectRole::SceneryObstacle) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        Obstacle obstacle{};
        obstacle.Handle = classified.Handle;
        obstacle.TagName = object.TagName;
        obstacle.Position = object.Position;
        obstacle.Forward = object.Forward;
        obstacle.Up = object.Up;
        obstacle.LinearVelocity = object.LinearVelocity;
        obstacle.AngularVelocity = object.AngularVelocity;
        obstacle.BoundingRadius = object.CurrentRadius;

        obstacles.push_back(obstacle);
    }
}

void System_Fixtures::CollectSpawns(const Classifieds& classifieds,
    const ObjectTable& objects, Spawns& spawns)
{
    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::Spawn) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const auto* scenery = std::get_if<SceneryObject>(&object.SpecificObject);
        if (!scenery || !scenery->Spawn.has_value()) continue;

        Spawn spawn{};
        spawn.Handle = classified.Handle;
        spawn.TagName = object.TagName;
        spawn.Position = object.Position;
        spawn.Forward = object.Forward;
        spawn.Team = scenery->Spawn->Team;

        if (scenery->Type == SceneryType::InitialSpawnPoint)
        {
            spawn.Type = SpawnType::Initial;
        }
        else if (scenery->Type == SceneryType::RespawnPoint)
        {
            spawn.Type = SpawnType::Respawn;
        }
        else if (scenery->Type == SceneryType::InvisibleRespawnPoint)
        {
            spawn.Type = SpawnType::Invisible;
        }

        spawns.push_back(spawn);
    }
}

void System_Fixtures::CollectTeleports(const Classifieds& classifieds,
    const ObjectTable& objects, Teleporters& teleporters)
{
    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::Teleporter) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
        if (!crate || !crate->Teleporter.has_value()) continue;

        Teleporter teleport{};
        teleport.Handle = classified.Handle;
        teleport.TagName = object.TagName;
        teleport.Position = object.Position;
        teleport.Forward = object.Forward;
        teleport.Up = object.Up;
        teleport.Channel = crate->Teleporter->Channel;
        teleport.ZoneShape = crate->Teleporter->ZoneShape;
        teleport.AllowedObjects = crate->Teleporter->AllowedObjects;

        if (crate->Type == CrateType::TeleporterReceiver)
        {
            teleport.Type = TeleporterType::Receiver;
        }
        else if (crate->Type == CrateType::TeleporterSender)
        {
            teleport.Type = TeleporterType::Sender;
        }
        else if (crate->Type == CrateType::TeleporterTwoWay)
        {
            teleport.Type = TeleporterType::TwoWay;
        }

        teleporters.push_back(teleport);
    }

    for (auto& teleporter : teleporters)
    {
        if (teleporter.Type == TeleporterType::Receiver) continue;

        TeleporterType targetRole = (teleporter.Type == TeleporterType::Sender) ?
            TeleporterType::Receiver : TeleporterType::TwoWay;

        for (const auto& other : teleporters)
        {
            if (other.Handle == teleporter.Handle) continue;
            if (other.Channel != teleporter.Channel) continue;
            if (other.Type != targetRole) continue;

            teleporter.DestinationPositions.push_back(other.Position);
        }
    }
}

void System_Fixtures::CollectLifts(const Classifieds& classifieds,
    const ObjectTable& objects, Lifts& lifts)
{
    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::Lift) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
        if (!crate || !crate->Lift.has_value()) continue;

        Lift lift{};
        lift.Handle = classified.Handle;
        lift.TagName = object.TagName;
        lift.Position = object.Position;
        lift.Forward = object.Forward;
        lift.Up = object.Up;
        lift.AngleType = crate->Lift->AngleType;
        lift.ForceType = crate->Lift->ForceType;

        switch (lift.AngleType)
        {
        case AngleType::Vertical:
            lift.LaunchDirection = { 0.0f, 0.0f, 1.0f };
            break;

        case AngleType::Curved:
            lift.LaunchDirection = lift.Forward;
            break;

        case AngleType::Redirected:
        {
            lift.LaunchDirection = {
                lift.Forward[1] * lift.Up[2] - lift.Forward[2] * lift.Up[1],
                lift.Forward[2] * lift.Up[0] - lift.Forward[0] * lift.Up[2],
                lift.Forward[0] * lift.Up[1] - lift.Forward[1] * lift.Up[0],
            };
            break;
        }
        default:
            lift.LaunchDirection = { 0.0f, 0.0f, 0.0f, };
        }

        lifts.push_back(lift);
    }
}

void System_Fixtures::CollectShields(const Classifieds& classifieds,
    const ObjectTable& objects, Shields& shields)
{
    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::Shield) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
        if (!crate || !crate->Shield.has_value()) continue;

        Shield shield{};
        shield.Handle = classified.Handle;
        shield.TagName = object.TagName;
        shield.Position = object.Position;
        shield.Forward = object.Forward;
        shield.Up = object.Up;
        shield.Type = crate->Shield->ShieldType;

        if (shield.Type == ShieldType::OneWay)
        {
            if (crate->Shield->IsShieldDoor)
            {
                shield.BlockDirection = {
                    -shield.Forward[0],
                    -shield.Forward[1],
                    -shield.Forward[2],
                };
            }
            else
            {
                shield.BlockDirection = shield.Forward;
            }
        }

        shields.push_back(shield);
    }
}

void System_Fixtures::CollectObjectives(const Classifieds& classifieds,
    const ObjectTable& objects, Objectives& objectives,
    ObjectiveSpawns& objectiveSpawns)
{
    for (const auto& classified : classifieds)
    {
        if (classified.Role == ObjectRole::ObjectiveSpawn)
        {
            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const LiveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
            if (!crate) continue;

            ObjectiveSpawn objectiveSpawn{};
            objectiveSpawn.Handle = classified.Handle;
            objectiveSpawn.TagName = object.TagName;
            objectiveSpawn.Position = object.Position;
            objectiveSpawn.Forward = object.Forward;
            objectiveSpawn.Up = object.Up;

            objectiveSpawn.ZoneShape = (crate->Zone.has_value()) ? 
                crate->Zone->Shape : ZoneShape{};

            objectiveSpawn.Team = (crate->Zone.has_value()) ?
                crate->Zone->Team : Team::Neutral;

            objectiveSpawns.push_back(objectiveSpawn);
        }
        else if (classified.Role == ObjectRole::ObjectivePickup ||
            classified.Role == ObjectRole::ObjectiveEquipped)
        {
            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const LiveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const auto* weap = std::get_if<WeaponObject>(&object.SpecificObject);
            if (!weap) continue;

            Objective objective{};
            objective.Handle = classified.Handle;
            objective.TagName = object.TagName;
            objective.Position = object.Position;
            objective.Forward = object.Forward;
            objective.Up = object.Up;
            objective.LinearVelocity = object.LinearVelocity;
            objective.AngularVelocity = object.AngularVelocity;

            objective.IsEquipped = (classified.Role == 
                ObjectRole::ObjectiveEquipped);

            if (objective.IsEquipped)
            {
                objective.CarrierHandle = object.ParentHandle;
            }

            if (weap->Team.has_value())
            {
                objective.Team = weap->Team.value();
            }

            objectives.push_back(objective);
        }
    }
}

void System_Fixtures::CollectDestructibles(const Classifieds& classifieds,
    const ObjectTable& objects, Destructibles& destructibles)
{
    for (const auto& classified : classifieds)
    {
        const bool isExplosive = (classified.Role == ObjectRole::Explosive);
        const bool isPallet = (classified.Role == ObjectRole::Pallet);
        const bool isPortable = (classified.Role == ObjectRole::PortableShield);

        if (!isExplosive && !isPallet && !isPortable) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        Destructible destructible{};
        destructible.Handle = classified.Handle;
        destructible.TagName = object.TagName;
        destructible.Position = object.Position;
        destructible.Forward = object.Forward;
        destructible.Up = object.Up;
        destructible.LinearVelocity = object.LinearVelocity;
        destructible.AngularVelocity = object.AngularVelocity;

        if (isExplosive)
        {
            destructible.Type = DestructibleType::Explosive;

            const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
            if (!crate || !crate->Destructible.has_value()) continue;
            destructible.Health = crate->Destructible->Health;
        }
        else if (isPallet)
        {
            destructible.Type = DestructibleType::Pallet;

            const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
            if (!crate || !crate->Destructible.has_value()) continue;
            destructible.Health = crate->Destructible->Health;
        }
        else if (isPortable)
        {
            destructible.Type = DestructibleType::PortableShield;

            const auto* scenery = std::get_if<SceneryObject>(&object.SpecificObject);
            if (!scenery || !scenery->PortableShield.has_value()) continue;
            destructible.Health = scenery->PortableShield->Health;
        }

        destructibles.push_back(destructible);
    }
}

void System_Fixtures::Cleanup()
{
    m_Deps.State_Fixtures.Cleanup();

    m_Deps.System_Logs.Log("[Fixtures] INFO: Cleanup completed.");
}