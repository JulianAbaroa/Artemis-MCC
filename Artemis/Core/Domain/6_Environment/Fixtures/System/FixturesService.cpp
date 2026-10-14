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
    using EquipmentObject = Tables::Object::Type::Equipment::Equipment;
    using CrateKind = Tables::Object::Type::Crate::Kind;
    using AngleType = Tables::Object::Type::Crate::Lift::Angle;
    using ShieldKind = Tables::Object::Type::Crate::Shield::Kind;
    using WeaponObject = Tables::Object::Type::Weapon::Weapon;
    using Role = Relations::Classifier::Type::Role;
    using Fixtures = Environment::Fixtures::Type::Fixtures;
    using BoundaryKind = Environment::Fixtures::Type::Boundary::Kind;
    using ZoneKind = Common::ZoneShape::Type::Kind;
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
        this->CollectZones(classifieds, objectTable, data.ObjectiveZones, data.Boundaries, data.Zones);
        this->CollectDestructibles(classifieds, objectTable, data.Destructibles);

        m_FixturesStore.Publish(std::move(data));
    }

    void FixturesService::CollectObstacles(const Classifieds& classifieds,
        const ObjectTable& objects, Obstacles& obstacles)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Role != Role::CrateObstacle &&
                classified.Role != Role::SceneryObstacle) continue;

            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const AliveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            Obstacle obstacle{};
            obstacle.Handle = classified.Handle;
            obstacle.TagName = object.TagName;
            obstacle.Position = object.BoundingCenter;
            obstacle.Forward = object.Forward;
            obstacle.Up = object.Up;
            obstacle.LinearVelocity = object.LinearVelocity;
            obstacle.AngularVelocity = object.AngularVelocity;
            obstacle.BoundingRadius = object.BoundingRadius;

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
            if (!scenery || !Tables::Object::Type::Scenery::IsSpawnPoint(scenery->Kind)) continue;

            Spawn spawn{};
            spawn.Handle = classified.Handle;
            spawn.TagName = object.TagName;
            spawn.Position = object.BoundingCenter;
            spawn.Forward = object.Forward;

            // note: Invisible respawn points are map defaults without a team.
            if (scenery->Kind == SceneryKind::InvisibleRespawnPoint) spawn.Team = Team::Neutral;
            else if (object.Multiplayer) spawn.Team = object.Multiplayer->Team;

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
            teleport.Position = object.WorldOrigin;
            teleport.Forward = object.Forward;
            teleport.Up = object.Up;
            teleport.Channel = crate->Teleport->Channel;
            teleport.ZoneShape = crate->Teleport->ZoneShape;
            teleport.Allowed = crate->Teleport->Allowed;

            using Allowed = Tables::Object::Type::Crate::Teleport::Allowed;
            const auto allowed = crate->Teleport->Allowed;
            teleport.Passes.Players = !Tables::Object::Type::Crate::Teleport::HasFlag(allowed, Allowed::NoPlayers);
            teleport.Passes.Ground = Tables::Object::Type::Crate::Teleport::HasFlag(allowed, Allowed::AllowGround);
            teleport.Passes.Heavy = Tables::Object::Type::Crate::Teleport::HasFlag(allowed, Allowed::AllowHeavy);
            teleport.Passes.Flying = Tables::Object::Type::Crate::Teleport::HasFlag(allowed, Allowed::AllowFlying);
            teleport.Passes.Projectiles = Tables::Object::Type::Crate::Teleport::HasFlag(allowed, Allowed::AllowProjctile);

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
            lift.Position = object.BoundingCenter;
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
            const auto* equipment = std::get_if<EquipmentObject>(&object.Specific);

            const auto* data = (crate && crate->Shield.has_value()) ? &*crate->Shield
                : (equipment && equipment->Shield.has_value()) ? &*equipment->Shield
                : nullptr;
            if (!data) continue;

            Shield shield{};
            shield.Handle = classified.Handle;
            shield.TagName = object.TagName;
            shield.Position = object.BoundingCenter;
            shield.Forward = object.Forward;
            shield.Up = object.Up;
            shield.Kind = data->Kind;

            if (shield.Kind == ShieldKind::OneWay)
            {
                if (data->IsShieldDoor)
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
                objectiveSpawn.Position = object.WorldOrigin;
                objectiveSpawn.Forward = object.Forward;
                objectiveSpawn.Up = object.Up;

                objectiveSpawn.Team = Team::Neutral;
                if (Tables::Object::Type::Crate::HasZone(crate->Kind))
                {
                    objectiveSpawn.ZoneShape = object.Multiplayer ? object.Multiplayer->Shape : ZoneShape{};
                    objectiveSpawn.Team = object.Multiplayer ? object.Multiplayer->Team : Team{};
                }

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
                objective.Position = object.BoundingCenter;
                objective.Forward = object.Forward;
                objective.Up = object.Up;
                objective.LinearVelocity = object.LinearVelocity;
                objective.AngularVelocity = object.AngularVelocity;

                objective.IsEquipped = (classified.Role == Role::ObjectiveEquipped);

                if (objective.IsEquipped)
                {
                    objective.CarrierHandle = object.ParentHandle;
                }

                if (object.Multiplayer) objective.Team = object.Multiplayer->Team;

                objectives.push_back(objective);
            }
        }
    }

    void FixturesService::CollectZones(const Classifieds& classifieds,
        const ObjectTable& objects, ObjectiveZones& objectiveZones,
        Boundaries& boundaries, Zones& zones)
    {
        for (const auto& classified : classifieds)
        {
            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const AliveObject& object = objectIt->second;
            if (object.Address == 0 || !object.Multiplayer.has_value()) continue;

            const ZoneShape& shape = object.Multiplayer->Shape;
            const Team team = object.Multiplayer->Team;

            if (classified.Role == Role::ObjectiveZone)
            {
                ObjectiveZone objectiveZone{};
                objectiveZone.Handle = classified.Handle;
                objectiveZone.TagName = object.TagName;
                objectiveZone.Position = object.WorldOrigin;
                objectiveZone.Forward = object.Forward;
                objectiveZone.Up = object.Up;
                objectiveZone.Team = team;
                objectiveZone.ZoneShape = shape;

                objectiveZones.push_back(objectiveZone);
            }
            else if (classified.Role == Role::Boundary)
            {
                const auto* scenery = std::get_if<SceneryObject>(&object.Specific);
                if (!scenery) continue;

                Boundary boundary{};
                boundary.Handle = classified.Handle;
                boundary.TagName = object.TagName;
                boundary.Position = object.WorldOrigin;
                boundary.Forward = object.Forward;
                boundary.Up = object.Up;
                boundary.Team = team;
                boundary.ZoneShape = shape;

                if (scenery->Kind == SceneryKind::SafeBoundary) boundary.Kind = BoundaryKind::Safe;
                else if (scenery->Kind == SceneryKind::SoftSafeBoundary) boundary.Kind = BoundaryKind::SoftSafe;
                else if (scenery->Kind == SceneryKind::KillBoundary) boundary.Kind = BoundaryKind::Kill;
                else if (scenery->Kind == SceneryKind::SoftKillBoundary) boundary.Kind = BoundaryKind::SoftKill;

                boundaries.push_back(boundary);
            }
            else if (classified.Role != Role::Teleporter && classified.Role != Role::ObjectiveSpawn)
            {
                // The teleporters and the objective spawns already carry their own zone.
                // note: Other kinds of shape (like the 1 seen on the bomb) are not drawable and are left out.
                if (shape.Kind != ZoneKind::Cylinder && shape.Kind != ZoneKind::Box) continue;

                Zone zone{};
                zone.Handle = classified.Handle;
                zone.TagName = object.TagName;
                zone.Position = object.WorldOrigin;
                zone.Forward = object.Forward;
                zone.Up = object.Up;
                zone.Team = team;
                zone.ZoneShape = shape;

                zones.push_back(zone);
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
            destructible.Position = object.BoundingCenter;
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