module Environment.Fixtures.System;

import Common.Semantic.Type;
import Common.Team.Type;

namespace
{
    using Team = Common::Team::Type::Team;
    using SceneryObject = Tables::Object::Type::Scenery::Scenery;
    using SceneryKind = Tables::Object::Type::Scenery::Kind;
    using CrateObject = Tables::Object::Type::Crate::Crate;
    using EquipmentObject = Tables::Object::Type::Equipment::Equipment;
    using AngleType = Tables::Object::Type::Crate::Lift::Angle;
    using ShieldKind = Tables::Object::Type::Crate::Shield::Kind;
    using WeaponObject = Tables::Object::Type::Weapon::Weapon;
    namespace Subtype = Common::Semantic::Type::Subtype;

    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Binding;
    using Fixtures = Environment::Fixtures::Type::Fixtures;
    using SpawnKind = Environment::Fixtures::Type::Spawn::Kind;
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
        this->CollectLifts(classifieds, objectTable, data.Lifts);
        this->CollectShields(classifieds, objectTable, data.Shields);
        this->CollectObjectives(classifieds, objectTable, data.Objectives);
        this->CollectDestructibles(classifieds, objectTable, data.Destructibles);

        m_FixturesStore.Publish(std::move(data));
    }

    void FixturesService::CollectObstacles(const Classifieds& classifieds,
        const ObjectTable& objects, Obstacles& obstacles)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Function != Function::Prop) continue;

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
            if (classified.Function != Function::SpawnPoint) continue;

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

    void FixturesService::CollectLifts(const Classifieds& classifieds,
        const ObjectTable& objects, Lifts& lifts)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Function != Function::Lift) continue;

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
            if (classified.Function != Function::Barrier) continue;

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
        const ObjectTable& objects, Objectives& objectives)
    {
        for (const auto& classified : classifieds)
        {
            if (classified.Function == Function::ObjectiveItem)
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

                objective.IsEquipped = (classified.Binding == Binding::Carried);

                if (objective.IsEquipped)
                {
                    objective.CarrierHandle = classified.BoundTo;
                }

                if (object.Multiplayer) objective.Team = object.Multiplayer->Team;

                objectives.push_back(objective);
            }
        }
    }

    void FixturesService::CollectDestructibles(const Classifieds& classifieds,
        const ObjectTable& objects, Destructibles& destructibles)
    {
        for (const auto& classified : classifieds)
        {
            // note: Pallets are props now. Neither they nor the explosives reach the list, since Tables never fills Crate::Destructible.
            // Kept until Fixtures is split in B4b.
            const bool isExplosive = (classified.Function == Function::Explosive);
            const bool isPallet = (classified.Function == Function::Prop);
            const bool isPortable = (classified.Function == Function::Barrier && classified.Subtype == static_cast<std::uint8_t>(Subtype::Barrier::Portable));

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