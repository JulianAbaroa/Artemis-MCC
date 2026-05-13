#include "pch.h"

// Header
#include "System_Navigation.h"

// Types.
#include "Core/Types/Domain/Map/MapMagics.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

// Map.
#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Map/Sbsp/State_MapSbsp.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"
#include "Core/States/Domain/Map/Bloc/State_MapBloc.h"
#include "Core/States/Domain/Map/Mach/State_MapMach.h"

// Object.
#include "Core/States/Domain/Object/State_ObjectTable.h"

// Classification.
#include "Core/States/Domain/Classification/State_Classification.h"

// Navigation.
#include "Core/States/Domain/Navigation/State_Navigation.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"

// Navigation.
#include "Sbsp/System_SbspGeometryBuilder.h"
#include "Sbsp/System_SbspSeamLinker.h"
#include "Scen/System_ScenObstacleBuilder.h"
#include "Bloc/System_BlocObstacleBuilder.h"
#include "Bloc/System_BlocTeleporterBuilder.h"
#include "Mach/System_MachDataBuilder.h"

// Debug.
#include "Core/Systems/Interface/System_Debug.h"

#include <array>

// Navigation, Out of cluster coverage
// TODO: Investigate engine raycast function, surface type, normals, ground distance.
// Would be the primary solution for positions outside cluster coverage.

// ----- Static Data -----

void System_Navigation::BuildForMap()
{
    auto& navigation = *g_pState->Domain->Navigation;
    auto& map = *g_pState->Domain->Map;
    auto& debug = *g_pSystem->Debug;

    std::vector<SbspGeometry> geometries;
    std::vector<const SbspObject*> sbspObjects;

    int32_t sbspCount = 0;
    int32_t scenCount = 0;
    int32_t blocObstacleCount = 0;
    int32_t blocTeleportCount = 0;
    int32_t machCount = 0;

    // We traverse all the tags, to build the needed data structures.
    // (Sbsp, Scen, Bloc and Mach)
    const int32_t tagCount = static_cast<int32_t>(map.GetTagsSize());
    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& entry = map.GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const std::string tagName = map.GetTagName(i);
        if (tagName.empty()) continue;

        const uint32_t magic = map.GetGroupMagic(entry.TagGroupIndex);

        // Build Sbsp.
        if (magic == MapMagics::k_SbspMagic)
        {
            if (!this->BuildSbsp(tagName, debug, 
                geometries, sbspObjects)) continue;
            ++sbspCount;
        }
        // Build Scen.
        else if (magic == MapMagics::k_ScenMagic)
        {
            if (!this->BuildScen(tagName, debug, navigation)) continue;
            ++scenCount;
        }
        // Build Bloc.
        else if (magic == MapMagics::k_BlocMagic)
        {
            auto [addedObstacle, addedTeleporter] =
                this->BuildBloc(tagName, debug, navigation);

            if (addedObstacle) ++blocObstacleCount;
            if (addedTeleporter) ++blocTeleportCount;
        }
        // Build Mach.
        else if (magic == MapMagics::k_MachMagic)
        {
            if (!this->BuildMach(tagName, debug, navigation)) continue;
            ++machCount;
        }
    }

    // If there's more than one SBSP, we link them all.
    if (sbspCount > 1)
    {
        auto& seamLinker = *g_pSystem->Domain->SbspSeamLinker;
        seamLinker.LinkSeams(geometries, sbspObjects);

        int32_t totalCrossLinks = 0;
        for (const auto& geometry : geometries)
        {
            for (const auto& cluster : geometry.Clusters)
            {
                totalCrossLinks += static_cast<int32_t>(
                    cluster.CrossLinks.size());
            }
        }

        debug.Log("[NavigationSystem] INFO: Seam linking complete."
            " Total cross-links: %d", totalCrossLinks);
    }

    // We save the all the SBSPs geometries obtained.
    for (auto& geometry : geometries) navigation.AddSbspGeometry(geometry);

    // We build the navigation graph.
    this->BuildNavigationGraph(geometries, navigation, debug);

    debug.Log("[NavigationSystem] INFO: Navigation built."
        " Total SBSPs: %d | Scen: %d | Bloc obstacle: %d |"
        " Bloc teleport: %d | Mach: %d", sbspCount, scenCount, 
        blocObstacleCount, blocTeleportCount, machCount);
}

bool System_Navigation::BuildSbsp(const std::string& tagName,
    System_Debug& debug, std::vector<SbspGeometry>& geometries,
    std::vector<const SbspObject*>& sbspObjects)
{
    if (tagName.find("shared") != std::string::npos) return false;
    if (tagName.find("hidden") != std::string::npos) return false;

    auto& mapSbsp = *g_pState->Domain->MapSbsp;
    const SbspObject* sbsp = mapSbsp.GetSbsp(tagName);
    if (!sbsp)
    {
        debug.Log("[NavigationSystem] WARNING:"
            " SBSP tag found in table but not loaded: ", tagName);
        return false;
    }

    auto& geomtryBuilder = *g_pSystem->Domain->SbspGeometryBuilder;
    SbspGeometry geometry = geomtryBuilder.BuildGeometry(*sbsp);

    debug.Log("[NavigationSystem] INFO:"
        " Built %d clusters, %d portals, %d markers, %d seams.",
        geometry.Clusters.size(), geometry.Portals.size(),
        geometry.Markers.size(), sbsp->StructureSeams.size());

    geometries.push_back(std::move(geometry));
    sbspObjects.push_back(sbsp);

    return true;
}

bool System_Navigation::BuildScen(const std::string& tagName,
    System_Debug& debug, State_Navigation& navigation)
{
    auto& mapScen = *g_pState->Domain->MapScen;
    const ScenObject* scen = mapScen.GetScen(tagName);
    if (!scen)
    {
        debug.Log("[NavigationSystem] WARNING:"
            " Scen tag found in table but not loaded: ", tagName);
        return false;
    }

    auto& obstacleBuilder = *g_pSystem->Domain->ScenObstacleBuilder;
    if (!obstacleBuilder.IsNavigationRelevant(*scen)) return false;

    SceneryObstacleData obstacle = obstacleBuilder.BuildData(*scen);
    navigation.AddScenObstacle(tagName, std::move(obstacle));

    return true;
}

BlocResult System_Navigation::BuildBloc(const std::string& tagName,
    System_Debug& debug, State_Navigation& navigation)
{
    auto& mapBloc = *g_pState->Domain->MapBloc;
    const BlocObject* bloc = mapBloc.GetBloc(tagName);
    if (!bloc)
    {
        debug.Log("[NavigationSystem] WARNING:"
            " Bloc tag found in table but not loaded: ", tagName);
        return {};
    }

    BlocResult result;

    auto& obstacleBuilder = *g_pSystem->Domain->BlocObstacleBuilder;
    if (obstacleBuilder.IsObstacleRelevant(*bloc))
    {
        CrateObstacleData obstacle = obstacleBuilder.BuildData(*bloc);
        navigation.AddBlocObstacle(tagName, std::move(obstacle));
        result.addedObstacle = true;
    }

    auto& teleporterBuilder = *g_pSystem->Domain->BlocTeleporterBuilder;
    if (teleporterBuilder.IsTeleporter(*bloc))
    {
        BlocTeleporterData teleporter = teleporterBuilder.BuildData(*bloc);
        navigation.AddBlocTeleporter(tagName, std::move(teleporter));
        result.addedTeleporter = true;
    }

    return result;
}

bool System_Navigation::BuildMach(const std::string& tagName, System_Debug& debug,
    State_Navigation& navigation)
{
    auto& mapMach = *g_pState->Domain->MapMach;
    const MachObject* mach = mapMach.GetMach(tagName);
    if (!mach)
    {
        debug.Log("[NavigationSystem] WARNING:"
            " Mach tag found in table but not loaded: ", tagName);
        return false;
    }

    auto& dataBuilder = *g_pSystem->Domain->MachDataBuilder;
    MachineData machine = dataBuilder.BuildData(*mach);
    navigation.AddMach(tagName, std::move(machine));

    return true;
}

void System_Navigation::BuildNavigationGraph(
    const std::vector<SbspGeometry>& geometries, 
    State_Navigation& navigation, System_Debug& debug) const
{
    std::vector<AINavigationCluster> clusters;
    std::vector<AINavigationCluster> rawClusters;
    int32_t skipped = 0;

    for (int32_t i = 0; i < static_cast<int32_t>(geometries.size()); ++i)
    {
        const SbspGeometry& geometry = geometries[i];

        for (const SbspCluster& cluster : geometry.Clusters)
        {
            float volume = (cluster.BoundsMax.X - cluster.BoundsMin.X) *
                (cluster.BoundsMax.Y - cluster.BoundsMin.Y) *
                (cluster.BoundsMax.Z - cluster.BoundsMin.Z);

            AINavigationCluster navigationCluster{};

            navigationCluster.ClusterIndex = cluster.ClusterIndex;
            navigationCluster.SbspIndex = i;

            navigationCluster.Center = { 
                cluster.Center.X, cluster.Center.Y, cluster.Center.Z 
            };

            navigationCluster.BoundsMin = { 
                cluster.BoundsMin.X, cluster.BoundsMin.Y, cluster.BoundsMin.Z 
            };

            navigationCluster.BoundsMax = { 
                cluster.BoundsMax.X, cluster.BoundsMax.Y, cluster.BoundsMax.Z 
            };

            for (int32_t portalIdx : cluster.PortalIndices)
            {
                if (portalIdx < 0 || portalIdx >= static_cast<int32_t>(
                    geometry.Portals.size())) continue;

                const SbspPortal& portal = geometry.Portals[portalIdx];

                int32_t neighborCluster = -1;

                if (portal.BackCluster == cluster.ClusterIndex)
                {
                    neighborCluster = portal.FrontCluster;
                }
                else if (portal.FrontCluster == cluster.ClusterIndex)
                {
                    neighborCluster = portal.BackCluster;
                }
                
                if (neighborCluster < 0) continue;

                AINavigationLink link{};

                link.ClusterIndex = neighborCluster;
                link.SbspIndex = i;

                link.Centroid = { 
                    portal.Centroid.X, portal.Centroid.Y, portal.Centroid.Z 
                };

                link.PassRadius = portal.BoundingRadius;

                navigationCluster.Links.push_back(link);
            }

            for (const SbspCrossLink& seam : cluster.CrossLinks)
            {
                AINavigationLink link{};

                link.ClusterIndex = seam.RemoteClusterIndex;
                link.SbspIndex = seam.RemoteSbspIndex;

                link.Centroid = { 
                    seam.ConnectionPoint.X, 
                    seam.ConnectionPoint.Y, 
                    seam.ConnectionPoint.Z 
                };

                link.PassRadius = 0.0f;

                navigationCluster.Links.push_back(link);
            }

            rawClusters.push_back(navigationCluster);

            if (volume <= m_MinVolume || volume >= m_MaxVolume)
            {
                ++skipped;
                continue;
            }

            clusters.push_back(std::move(navigationCluster));
        }
    }

    const int32_t total = static_cast<int32_t>(clusters.size());
    navigation.SetNavigationGraph(std::move(clusters));
    navigation.SetRawNavigationGraph(std::move(rawClusters));

    debug.Log("[NavigationSystem] INFO: Navigation graph built."
        " Total clusters: %d | Skipped: %d", total, skipped);
}

// ----- Dynamic Data -----

void System_Navigation::UpdateNavigation()
{
    auto& navigation = *g_pState->Domain->Navigation;
    auto& classifieds = g_pState->Domain->Classification->GetObjects();
    auto& objects = g_pState->Domain->ObjectTable->GetObjectTable();

    // Obstacles.
    this->BuildObstacles(navigation, classifieds, objects);

    // Spawns.
    this->BuildSpawns(navigation, classifieds, objects);

    // Teleports.
    this->BuildTeleports(navigation, classifieds, objects);

    // Lifts.
    this->BuildLifts(navigation, classifieds, objects);

    // Shields.
    this->BuildShields(navigation, classifieds, objects);

    // Objectives.
    this->BuildObjectives(navigation, classifieds, objects);

    // Explosives.
    this->BuildDestructibles(navigation, classifieds, objects);
}

void System_Navigation::BuildObstacles(State_Navigation& navigation,
    const std::vector<ClassifiedObject>& classifieds,
    const std::unordered_map<uint32_t, LiveObject> objects)
{
    std::vector<ActiveObstacle> obstacles;

    // Bloc.
    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::CrateObstacle) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        ActiveObstacle obstacle{};

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

    // Scen.
    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::SceneryObstacle) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        ActiveObstacle obstacle{};

        obstacle.Handle = classified.Handle;
        obstacle.TagName = object.TagName;

        obstacle.Position = object.Position;
        obstacle.Forward = object.Forward;
        obstacle.Up = object.Up;

        obstacle.BoundingRadius = object.CurrentRadius;

        obstacles.push_back(obstacle);
    }

    navigation.SetActiveObstacles(std::move(obstacles));
}

void System_Navigation::BuildSpawns(State_Navigation& navigation,
    const std::vector<ClassifiedObject>& classifieds,
    const std::unordered_map<uint32_t, LiveObject> objects)
{
    std::vector<ActiveSpawn> spawns;

    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::Spawn) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const auto* scenery = 
            std::get_if<SceneryObject>(&object.SpecificObject);
        if (!scenery) continue;

        if (!scenery->Spawn.has_value()) continue;

        ActiveSpawn spawn{};

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

    navigation.SetActiveSpawns(std::move(spawns));
}

void System_Navigation::BuildTeleports(State_Navigation& navigation,
    const std::vector<ClassifiedObject>& classifieds,
    const std::unordered_map<uint32_t, LiveObject> objects)
{
    std::vector<ActiveTeleporter> teleporters;

    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::Teleporter) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
        if (!crate) continue;

        if (!crate->Teleporter.has_value()) continue;

        ActiveTeleporter teleport{};

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

    // Resolve destination positions for each teleporter.
    for (auto& teleporter : teleporters)
    {
        if (teleporter.Type == TeleporterType::Receiver) continue;

        TeleporterType targetRole = 
            (teleporter.Type == TeleporterType::Sender) ? 
            TeleporterType::Receiver : TeleporterType::TwoWay;

        for (const auto& other : teleporters)
        {
            if (other.Handle == teleporter.Handle) continue;
            if (other.Channel != teleporter.Channel) continue;
            if (other.Type != targetRole) continue;

            teleporter.DestinationPositions.push_back(other.Position);
        }
    }

    navigation.SetActiveTeleporters(std::move(teleporters));
}

void System_Navigation::BuildLifts(State_Navigation& navigation,
    const std::vector<ClassifiedObject>& classifieds,
    const std::unordered_map<uint32_t, LiveObject> objects)
{
    std::vector<ActiveLift> lifts;

    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::Lift) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
        if (!crate) continue;

        if (!crate->Lift.has_value()) continue;

        ActiveLift lift{};

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

    navigation.SetActiveLifts(std::move(lifts));
}

void System_Navigation::BuildShields(State_Navigation& navigation,
    const std::vector<ClassifiedObject>& classifieds,
    const std::unordered_map<uint32_t, LiveObject> objects)
{
    std::vector<ActiveShield> shields;

    for (const auto& classified : classifieds)
    {
        if (classified.Role != ObjectRole::Shield) continue;

        auto objectIt = objects.find(classified.Handle);
        if (objectIt == objects.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
        if (!crate) continue;

        if (!crate->Shield.has_value()) continue;

        ActiveShield shield{};

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

    navigation.SetActiveShields(std::move(shields));
}

void System_Navigation::BuildObjectives(State_Navigation& navigation,
    const std::vector<ClassifiedObject>& classifieds,
    const std::unordered_map<uint32_t, LiveObject> objects)
{
    std::vector<ActiveObjectiveSpawn> objectiveSpawns;
    std::vector<ActiveObjective> objectives;

    for (const auto& classified : classifieds)
    {
        if (classified.Role == ObjectRole::ObjectiveSpawn)
        {
            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const LiveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const auto* crate = 
                std::get_if<CrateObject>(&object.SpecificObject);

            ActiveObjectiveSpawn spawn{};

            spawn.Handle = classified.Handle;
            spawn.TagName = object.TagName;

            spawn.Position = object.Position;
            spawn.Forward = object.Forward;
            spawn.Up = object.Up;

            spawn.ZoneShape = crate->Zone->Shape;

            spawn.Team = (crate && crate->Zone.has_value()) ? 
                crate->Zone->Team : Team::Neutral;

            objectiveSpawns.push_back(spawn);
        }
        else if (classified.Role == ObjectRole::ObjectivePickup ||
            classified.Role == ObjectRole::ObjectiveEquiped)
        {
            auto objectIt = objects.find(classified.Handle);
            if (objectIt == objects.end()) continue;

            const LiveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const auto* weap = std::get_if<WeaponObject>(&object.SpecificObject);

            ActiveObjective objective{};

            objective.Handle = classified.Handle;
            objective.TagName = object.TagName;

            objective.Position = object.Position;
            objective.Forward = object.Forward;
            objective.Up = object.Up;
            objective.LinearVelocity = object.LinearVelocity;
            objective.AngularVelocity = object.AngularVelocity;

            objective.IsEquipped = 
                (classified.Role == ObjectRole::ObjectiveEquiped);

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

    navigation.SetActiveObjectiveSpawns(std::move(objectiveSpawns));
    navigation.SetActiveObjectives(std::move(objectives));
}

void System_Navigation::BuildDestructibles(State_Navigation& navigation,
    const std::vector<ClassifiedObject>& classifieds,
    const std::unordered_map<uint32_t, LiveObject> objects)
{
    std::vector<ActiveDestructible> destructibles;

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

        ActiveDestructible dest{};

        dest.Handle = classified.Handle;
        dest.TagName = object.TagName;
        dest.Position = object.Position;
        dest.Forward = object.Forward;
        dest.Up = object.Up;
        dest.LinearVelocity = object.LinearVelocity;
        dest.AngularVelocity = object.AngularVelocity;

        if (isExplosive)
        {
            dest.Type = DestructibleType::Explosive;

            const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
            if (!crate || !crate->Destructible.has_value()) continue;
            dest.Health = crate->Destructible->Health;
        }
        else if (isPallet)
        {
            dest.Type = DestructibleType::Pallet;

            const auto* crate = std::get_if<CrateObject>(&object.SpecificObject);
            if (!crate || !crate->Destructible.has_value()) continue;
            dest.Health = crate->Destructible->Health;
        }
        else if (isPortable)
        {
            dest.Type = DestructibleType::PortableShield;

            const auto* scenery = std::get_if<SceneryObject>(&object.SpecificObject);
            if (!scenery || !scenery->PortableShield.has_value()) continue;
            dest.Health = scenery->PortableShield->Health;
        }

        destructibles.push_back(dest);
    }

    navigation.SetActiveDestructibles(std::move(destructibles));
}

// Cleanup.
void System_Navigation::Cleanup()
{
    g_pState->Domain->Navigation->Cleanup();
    g_pSystem->Debug->Log("[NavigationSystem] INFO: Cleanup completed.");
}