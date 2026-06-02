#pragma once

#include "Core/Types/Navigation/NavigationTypes.h"
#include "Core/Types/Navigation/SbspGeometry.h"
#include "Core/Types/Navigation/ScnrZones.h"

#include <unordered_map>
#include <utility>
#include <string>
#include <vector>
#include <atomic>
#include <array>
#include <mutex>
#include <tuple>

class State_Navigation
{
public:
    // --- Static Data ---

    // Sbsp.
    bool HasSbspGeometry() const;
    const SbspGeometry* GetSbspGeometry(int32_t index) const;
    const std::vector<SbspGeometry>& GetAllSbspGeometry() const;
    void AddSbspGeometry(SbspGeometry geometry);
    int32_t GetSbspGeometryCount() const;

    // Scnr.
    bool HasMapZones() const;
    const ScnrMapZones* GetMapZones() const;
    void SetMapZones(ScnrMapZones zones);

    // Navigation graph.
    const std::vector<AINavigationCluster> GetNavigationGraph() const;
    void SetNavigationGraph(std::vector<AINavigationCluster> clusters);

    // --- Dynamic Data ---

    // Obstacles.
    std::vector<ActiveObstacle> GetActiveObstacles() const;
    void SetActiveObstacles(std::vector<ActiveObstacle> obstacles);

    // Spawns.
    std::vector<ActiveSpawn> GetActiveSpawns() const;
    void SetActiveSpawns(std::vector<ActiveSpawn> spawns);

    // Teleports.
    std::vector<ActiveTeleporter> GetActiveTeleporters() const;
    void SetActiveTeleporters(std::vector<ActiveTeleporter> teleporters);

    // Lifts.
    std::vector<ActiveLift> GetActiveLifts() const;
    void SetActiveLifts(std::vector<ActiveLift> lifts);

    // Shields.
    std::vector<ActiveShield> GetActiveShields() const;
    void SetActiveShields(std::vector<ActiveShield> shields);

    // Objective Spawns.
    std::vector<ActiveObjectiveSpawn> GetActiveObjectiveSpawns() const;
    void SetActiveObjectiveSpawns(std::vector<ActiveObjectiveSpawn> spawns);

    // Objectives.
    std::vector<ActiveObjective> GetActiveObjectives() const;
    void SetActiveObjectives(std::vector<ActiveObjective> objectives);

    // Destructibles.
    std::vector<ActiveDestructible> GetActiveDestructibles() const;
    void SetActiveDestructibles(std::vector<ActiveDestructible> destructibles);

    void Cleanup();

private:
    // --- Static Data ---

    std::vector<SbspGeometry> m_SbspGeometries;
    std::vector<AINavigationCluster> m_NavigationGraph;
    std::vector<AINavigationCluster> m_RawNavigationGraph;

    ScnrMapZones m_MapZones;
    std::atomic<bool> m_HasMapZones;

    // --- Dynamic Data ---

    std::vector<ActiveObstacle> m_ActiveObstacles;
    std::vector<ActiveSpawn> m_ActiveSpawns;
    std::vector<ActiveTeleporter> m_ActiveTeleporters;
    std::vector<ActiveLift> m_ActiveLifts;
    std::vector<ActiveShield> m_ActiveShields;
    std::vector<ActiveObjectiveSpawn> m_ActiveObjectiveSpawns;
    std::vector<ActiveObjective> m_ActiveObjectives;
    std::vector<ActiveDestructible> m_ActiveDestructibles;

    mutable std::mutex m_Mutex;
};