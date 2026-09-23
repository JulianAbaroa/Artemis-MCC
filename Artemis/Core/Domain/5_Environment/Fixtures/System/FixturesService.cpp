module Environment.Fixtures.System;

import Common.Team.Type;
import Common.ZoneShape.Type;

namespace
{
    using Team = Common::Team::Type::Team;
    using ZoneShape = Common::ZoneShape::Type::ZoneShape;
    using SceneryObject = Tables::Object::Type::Scenery::Scenery;
    using SceneryKind = Tables::Object::Type::Scenery::Kind;
    using CrateObject = Tables::Object::Type::Crate::Crate;
    using CrateKind = Tables::Object::Type::Crate::Kind;
    using AngleType = Tables::Object::Type::Crate::Lift::Angle;
    using ShieldKind = Tables::Object::Type::Crate::Shield::Kind;
    using WeaponObject = Tables::Object::Type::Weapon::Weapon;
    using Role = Relations::Classifier::Type::Role;
    using Fixtures = Environment::Fixtures::Type::Fixtures;
    using SpawnKind = Environment::Fixtures::Type::Spawn::Kind;
    using TeleportKind = Environment::Fixtures::Type::Teleport::Kind;
    using DestructibleKind = Environment::Fixtures::Type::Destructible::Kind;
}

namespace Environment::Fixtures::System
{
    void FixturesService::Update()
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        const Classifieds& classifieds = *classifiedsPtr;
        const ObjectTable& objectTable = *objectTablePtr;

        ::Fixtures data;
        this->CollectObstacles(classifieds, objectTable, data.Obstacles);
        this->CollectSpawns(classifieds, objectTable, data.Spawns);
        this->CollectTeleports(classifieds, objectTable, data.Teleporters);
        this->CollectLifts(classifieds, objectTable, data.Lifts);
        this->CollectShields(classifieds, objectTable, data.Shields);
        this->CollectObjectives(classifieds, objectTable, data.Objectives, data.ObjectiveSpawns);
        this->CollectDestructibles(classifieds, objectTable, data.Destructibles);

        m_FixturesStore.Publish(std::move(data));
    }

    void FixturesService::CollectObstacles(const Classifieds& classifieds,
        const ObjectTable& objects, Obstacles& obstacles)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Role != Role::CrateObstacle ||
                classified.Role != Role::SceneryObstacle) continue;

            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const AliveObject& object = objectIt->second;
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

    void FixturesService::CollectSpawns(const Classifieds& classifieds,
        const ObjectTable& objects, Spawns& spawns)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Role != Role::Spawn) continue;

            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const AliveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const auto* scenery = std::get_if<SceneryObject>(&object.Specific);
            if (!scenery || !scenery->Spawn.has_value()) continue;

            Spawn spawn{};
            spawn.Handle = classified.Handle;
            spawn.TagName = object.TagName;
            spawn.Position = object.Position;
            spawn.Forward = object.Forward;
            spawn.Team = scenery->Spawn->Team;

            if (scenery->Kind == SceneryKind::InitialSpawnPoint)
            {
                spawn.Kind = SpawnKind::Initial;
            }
            else if (scenery->Kind == SceneryKind::RespawnPoint)
            {
                spawn.Kind = SpawnKind::Respawn;
            }
            else if (scenery->Kind == SceneryKind::InvisibleRespawnPoint)
            {
                spawn.Kind = SpawnKind::Invisible;
            }

            spawns.push_back(spawn);
        }
    }

    void FixturesService::CollectTeleports(const Classifieds& classifieds,
        const ObjectTable& objects, Teleporters& teleporters)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Role != Role::Teleporter) continue;

            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const AliveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const auto* crate = std::get_if<CrateObject>(&object.Specific);
            if (!crate || !crate->Teleport.has_value()) continue;

            Teleport teleport{};
            teleport.Handle = classified.Handle;
            teleport.TagName = object.TagName;
            teleport.Position = object.Position;
            teleport.Forward = object.Forward;
            teleport.Up = object.Up;
            teleport.Channel = crate->Teleport->Channel;
            teleport.ZoneShape = crate->Teleport->ZoneShape;
            teleport.Allowed = crate->Teleport->Allowed;

            if (crate->Kind == CrateKind::TeleportReceiver)
            {
                teleport.Kind = TeleportKind::Receiver;
            }
            else if (crate->Kind == CrateKind::TeleportSender)
            {
                teleport.Kind = TeleportKind::Sender;
            }
            else if (crate->Kind == CrateKind::TeleportTwoWay)
            {
                teleport.Kind = TeleportKind::TwoWay;
            }

            teleporters.push_back(teleport);
        }

        for (auto& teleporter : teleporters)
        {
            if (teleporter.Kind == TeleportKind::Receiver) continue;

            TeleportKind targetRole = (teleporter.Kind == TeleportKind::Sender) ?
                TeleportKind::Receiver : TeleportKind::TwoWay;

            for (const auto& other : teleporters)
            {
                if (other.Handle == teleporter.Handle) continue;
                if (other.Channel != teleporter.Channel) continue;
                if (other.Kind != targetRole) continue;

                teleporter.DestinationPositions.push_back(other.Position);
            }
        }
    }

    void FixturesService::CollectLifts(const Classifieds& classifieds,
        const ObjectTable& objects, Lifts& lifts)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Role != Role::Lift) continue;

            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const AliveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const auto* crate = std::get_if<CrateObject>(&object.Specific);
            if (!crate || !crate->Lift.has_value()) continue;

            Lift lift{};
            lift.Handle = classified.Handle;
            lift.TagName = object.TagName;
            lift.Position = object.Position;
            lift.Forward = object.Forward;
            lift.Up = object.Up;
            lift.Angle = crate->Lift->Angle;
            lift.Force = crate->Lift->Force;

            switch (lift.Angle)
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
                    lift.Forward.Y * lift.Up.Z - lift.Forward.Z * lift.Up.Y,
                    lift.Forward.Z * lift.Up.X - lift.Forward.X * lift.Up.Z,
                    lift.Forward.X * lift.Up.Y - lift.Forward.Y * lift.Up.X,
                };
                break;
            }
            default:
                lift.LaunchDirection = { 0.0f, 0.0f, 0.0f, };
            }

            lifts.push_back(lift);
        }
    }

    void FixturesService::CollectShields(const Classifieds& classifieds,
        const ObjectTable& objects, Shields& shields)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Role != Role::Shield) continue;

            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const AliveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const auto* crate = std::get_if<CrateObject>(&object.Specific);
            if (!crate || !crate->Shield.has_value()) continue;

            Shield shield{};
            shield.Handle = classified.Handle;
            shield.TagName = object.TagName;
            shield.Position = object.Position;
            shield.Forward = object.Forward;
            shield.Up = object.Up;
            shield.Kind = crate->Shield->Kind;

            if (shield.Kind == ShieldKind::OneWay)
            {
                if (crate->Shield->IsShieldDoor)
                {
                    shield.BlockDirection = {
                        -shield.Forward.X,
                        -shield.Forward.Y,
                        -shield.Forward.Z,
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

    void FixturesService::CollectObjectives(const Classifieds& classifieds,
        const ObjectTable& objects, Objectives& objectives,
        ObjectiveSpawns& objectiveSpawns)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Role == Role::ObjectiveSpawn)
            {
                auto objectIt = objects.find(classified.Handle);
                if (objectIt == objects.end()) continue;

                const AliveObject& object = objectIt->second;
                if (object.Address == 0) continue;

                const auto* crate = std::get_if<CrateObject>(&object.Specific);
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
            else if (classified.Role == Role::ObjectivePickup ||
                classified.Role == Role::ObjectiveEquipped)
            {
                auto objectIt = objects.find(classified.Handle);
                if (objectIt == objects.end()) continue;

                const AliveObject& object = objectIt->second;
                if (object.Address == 0) continue;

                const auto* weap = std::get_if<WeaponObject>(&object.Specific);
                if (!weap) continue;

                Objective objective{};
                objective.Handle = classified.Handle;
                objective.TagName = object.TagName;
                objective.Position = object.Position;
                objective.Forward = object.Forward;
                objective.Up = object.Up;
                objective.LinearVelocity = object.LinearVelocity;
                objective.AngularVelocity = object.AngularVelocity;

                objective.IsEquipped = (classified.Role == Role::ObjectiveEquipped);

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

    void FixturesService::CollectDestructibles(const Classifieds& classifieds,
        const ObjectTable& objects, Destructibles& destructibles)
    {
        for (const auto& classified : classifieds)
        {
            const bool isExplosive = (classified.Role == Role::Explosive);
            const bool isPallet = (classified.Role == Role::Pallet);
            const bool isPortable = (classified.Role == Role::PortableShield);

            if (!isExplosive && !isPallet && !isPortable) continue;

            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const AliveObject& object = objectIt->second;
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
                destructible.Kind = DestructibleKind::Explosive;

                const auto* crate = std::get_if<CrateObject>(&object.Specific);
                if (!crate || !crate->Destructible.has_value()) continue;
                destructible.Health = crate->Destructible->Health;
            }
            else if (isPallet)
            {
                destructible.Kind = DestructibleKind::Pallet;

                const auto* crate = std::get_if<CrateObject>(&object.Specific);
                if (!crate || !crate->Destructible.has_value()) continue;
                destructible.Health = crate->Destructible->Health;
            }
            else if (isPortable)
            {
                destructible.Kind = DestructibleKind::PortableShield;

                const auto* scenery = std::get_if<SceneryObject>(&object.Specific);
                if (!scenery || !scenery->PortableShield.has_value()) continue;
                destructible.Health = scenery->PortableShield->Health;
            }

            destructibles.push_back(destructible);
        }
    }

    void FixturesService::Cleanup()
    {
        m_FixturesStore.Cleanup();

        m_LogsService.Message("[FixturesService] INFO: Cleanup completed.");
    }
}