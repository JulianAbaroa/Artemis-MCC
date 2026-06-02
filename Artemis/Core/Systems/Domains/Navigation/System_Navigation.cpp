#include "pch.h"

#include "System_Navigation.h"

#include "Core/Types/Map/MapMagics.h"

#include "Core/States/MapReader/State_MapReader.h"
#include "Core/States/MapReader/Sbsp/State_MapSbsp.h"
#include "Core/States/MapReader/Scen/State_MapScen.h"
#include "Core/States/MapReader/Bloc/State_MapBloc.h"
#include "Core/States/MapReader/Mach/State_MapMach.h"
#include "Core/States/MapReader/Scnr/State_MapScnr.h"
#include "Core/States/Tables/Object/State_ObjectTable.h"
#include "Core/States/Filtered/Classifier/State_Classifier.h"
#include "Core/States/Domains/Navigation/State_Navigation.h"

#include "Core/Systems/MapReader/Mesh/System_MeshReader.h"
#include "Sbsp/System_SbspBuilder.h"
#include "Sbsp/System_SbspSeamLinker.h"
#include "Scnr/System_ScnrBuilder.h"
#include "Core/Systems/Logs/System_Logs.h"

#include <array>

// TODO: BuildBoundaries(), BuildSpawnZones().

// ----- Static Data -----

void System_Navigation::BuildForMap()
{
    std::vector<SbspGeometry> geometries;
    std::vector<const SbspObject*> sbspObjects;

    int32_t sbspCount = 0;
    int32_t scenCount = 0;
    int32_t blocObstacleCount = 0;
    int32_t blocTeleportCount = 0;
    int32_t machCount = 0;
    int32_t scnrCount = 0;

    // We traverse all the tags, to build the needed data structures.
    // (Sbsp, Scen, Bloc and Mach)
    const int32_t tagCount = static_cast<int32_t>(m_Deps.State_Map.GetTagsSize());
    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& entry = m_Deps.State_Map.GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const std::string tagName = m_Deps.State_Map.GetTagName(i);
        if (tagName.empty()) continue;

        const uint32_t magic =
            m_Deps.State_Map.GetGroupMagic(entry.TagGroupIndex);

        // Build Sbsp.
        if (magic == MapMagics::k_SbspMagic)
        {
            if (!this->BuildSbsp(tagName, geometries, sbspObjects)) continue;
            ++sbspCount;
        }
        else if (magic == MapMagics::k_ScnrMagic)
        {
            if (!this->BuildScnr(tagName)) continue;
            ++scnrCount;
        }
    }

    // Read complete render geometry of the sbsp/lbsp/zone/play tags.
    bool readed = m_Deps.System_MeshReader.ReadSbspGeometries(geometries);
    if (!readed)
    {
        m_Deps.System_Logs.Log("[NavigationSystem] ERROR:"
            " Failed to read SBSP geometries.");
        return;
    }

    // If there's more than one SBSP, we link them all.
    if (sbspCount > 1)
    {
        m_Deps.System_SbspSeamLinker.LinkSeams(geometries, sbspObjects);

        int32_t totalCrossLinks = 0;
        for (const auto& geometry : geometries)
        {
            for (const auto& cluster : geometry.Clusters)
            {
                totalCrossLinks += static_cast<int32_t>(
                    cluster.CrossLinks.size());
            }
        }

        m_Deps.System_Logs.Log("[NavigationSystem] INFO: Seam linking complete."
            " Total cross-links: %d", totalCrossLinks);
    }

    // We save the all the SBSPs geometries obtained.
    for (auto& geometry : geometries)
    {
        m_Deps.State_Navigation.AddSbspGeometry(geometry);
    }

    // We build the navigation graph.
    this->BuildNavigationGraph(geometries);

    m_Deps.System_Logs.Log("[NavigationSystem] INFO: Navigation built."
        " Total SBSPs: %d | Scen: %d | Bloc obstacle: %d |"
        " Bloc teleport: %d | Mach: %d | Scnr: %d", sbspCount, scenCount, 
        blocObstacleCount, blocTeleportCount, machCount, scnrCount);
}

bool System_Navigation::BuildSbsp(const std::string& tagName, 
    std::vector<SbspGeometry>& geometries,
    std::vector<const SbspObject*>& sbspObjects)
{
    if (tagName.find("shared") != std::string::npos) return false;
    if (tagName.find("hidden") != std::string::npos) return false;

    const SbspObject* sbsp = m_Deps.State_MapSbsp.GetSbsp(tagName);
    if (!sbsp)
    {
        m_Deps.System_Logs.Log("[NavigationSystem] WARNING:"
            " SBSP tag found in table but not loaded: ", tagName);
        return false;
    }

    int32_t sbspIndex = static_cast<int32_t>(geometries.size());
    SbspGeometry geometry = m_Deps.System_SbspBuilder
        .Build(*sbsp, sbspIndex);

    m_Deps.System_Logs.Log("[NavigationSystem] INFO:"
        " Built %d clusters, %d portals, %d markers, %d seams.",
        geometry.Clusters.size(), geometry.Portals.size(),
        geometry.Markers.size(), sbsp->StructureSeams.size());

    geometries.push_back(std::move(geometry));
    sbspObjects.push_back(sbsp);

    return true;
}

bool System_Navigation::BuildScnr(const std::string& tagName)
{
    const ScnrObject* scnr = m_Deps.State_MapScnr.GetScnr(tagName);
    if (!scnr)
    {
        m_Deps.System_Logs.Log("[EnvironmentSystem] WARNING: "
            " Scnr tag found in table but not loaded: ", tagName);
        return false;
    }

    ScnrMapZones zones = m_Deps.System_ScnrBuilder.Build(*scnr);

    m_Deps.State_Navigation.SetMapZones(std::move(zones));
    return true;
}

void System_Navigation::BuildNavigationGraph(
    const std::vector<SbspGeometry>& geometries) const
{
    std::vector<AINavigationCluster> clusters;

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

            clusters.push_back(std::move(navigationCluster));
        }
    }

    // Mark kill/safe zones.
    const ScnrMapZones* zones = m_Deps.State_Navigation.GetMapZones();
    if (zones)
    {
        for (auto& cluster : clusters)
        {
            for (const auto& kz : zones->KillZones)
            {
                if (this->ClusterOverlapsVolume(cluster, kz))
                {
                    cluster.IsKillZone = true;
                    break;
                }
            }
            for (const auto& sz : zones->SafeZones)
            {
                if (this->ClusterOverlapsVolume(cluster, sz))
                {
                    cluster.IsSafeZone = true;
                    break;
                }
            }
        }
    }

    const int32_t total = static_cast<int32_t>(clusters.size());
    m_Deps.State_Navigation.SetNavigationGraph(std::move(clusters));

    m_Deps.System_Logs.Log("[NavigationSystem] INFO: Navigation graph built."
        " Total clusters: %d", total);
}

bool System_Navigation::ClusterOverlapsVolume(
    const AINavigationCluster& cluster, 
    const ScnrTriggerVolume& volume) const
{
    float vMinX = volume.Position[0] - volume.Extents[0];
    float vMaxX = volume.Position[0] + volume.Extents[0];
    float vMinY = volume.Position[1] - volume.Extents[1];
    float vMaxY = volume.Position[1] + volume.Extents[1];
    float vMinZ = volume.Position[2] - volume.Extents[2];
    float vMaxZ = volume.Position[2] + volume.Extents[2];

    return cluster.BoundsMin[0] <= vMaxX && cluster.BoundsMax[0] >= vMinX &&
        cluster.BoundsMin[1] <= vMaxY && cluster.BoundsMax[1] >= vMinY &&
        cluster.BoundsMin[2] <= vMaxZ && cluster.BoundsMax[2] >= vMinZ;
}

// ----- Dynamic Data -----

void System_Navigation::UpdateNavigation()
{
    auto& classifieds = m_Deps.State_Classification.GetClassifieds();
    auto& objects = m_Deps.State_ObjectTable.GetObjectTable();

    // Obstacles.
    this->BuildObstacles(classifieds, objects);

    // Spawns.
    this->BuildSpawns(classifieds, objects);

    // Teleports.
    this->BuildTeleports(classifieds, objects);

    // Lifts.
    this->BuildLifts(classifieds, objects);

    // Shields.
    this->BuildShields(classifieds, objects);

    // Objectives.
    this->BuildObjectives(classifieds, objects);

    // Explosives.
    this->BuildDestructibles(classifieds, objects);
}

void System_Navigation::BuildObstacles(
    const std::vector<Classified>& classifieds,
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

    m_Deps.State_Navigation.SetActiveObstacles(std::move(obstacles));
}

void System_Navigation::BuildSpawns(
    const std::vector<Classified>& classifieds,
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

    m_Deps.State_Navigation.SetActiveSpawns(std::move(spawns));
}

void System_Navigation::BuildTeleports(
    const std::vector<Classified>& classifieds,
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

    m_Deps.State_Navigation.SetActiveTeleporters(std::move(teleporters));
}

void System_Navigation::BuildLifts(
    const std::vector<Classified>& classifieds,
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

    m_Deps.State_Navigation.SetActiveLifts(std::move(lifts));
}

void System_Navigation::BuildShields(
    const std::vector<Classified>& classifieds,
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

    m_Deps.State_Navigation.SetActiveShields(std::move(shields));
}

void System_Navigation::BuildObjectives(
    const std::vector<Classified>& classifieds,
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

    m_Deps.State_Navigation.SetActiveObjectiveSpawns(
        std::move(objectiveSpawns));

    m_Deps.State_Navigation.SetActiveObjectives(std::move(objectives));
}

void System_Navigation::BuildDestructibles(
    const std::vector<Classified>& classifieds,
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

    m_Deps.State_Navigation.SetActiveDestructibles(std::move(destructibles));
}

void System_Navigation::Cleanup()
{
    m_Deps.State_Navigation.Cleanup();
    m_Deps.System_Logs.Log("[NavigationSystem] INFO: Cleanup completed.");
}