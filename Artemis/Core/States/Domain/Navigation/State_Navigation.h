#pragma once

// Types.
#include "Core/Types/Domain/Navigation/NavigationTypes.h"
#include "Core/Types/Domain/Navigation/SbspGeometry.h"
#include "Core/Types/Domain/Navigation/ScenObstacleData.h"
#include "Core/Types/Domain/Navigation/BlocObstacleData.h"
#include "Core/Types/Domain/Navigation/BlocTeleporterData.h"
#include "Core/Types/Domain/Navigation/MachData.h"

#include <unordered_map>
#include <utility>
#include <string>
#include <vector>
#include <array>
#include <mutex>
#include <tuple>

class State_Navigation
{
public:
    // ----- Static Data -----

    // Sbsp.
    bool HasSbspGeometry() const;
    const SbspGeometry* GetSbspGeometry(int32_t index) const;
    const std::vector<SbspGeometry>& GetAllSbspGeometry() const;
    void AddSbspGeometry(SbspGeometry geometry);
    int32_t GetSbspGeometryCount() const;

    // Scen.
    bool HasScenObstacle() const;
    const SceneryObstacleData* GetScenObstacle(const std::string& tagName) const;
    void AddScenObstacle(const std::string& tagName, SceneryObstacleData obstacle);

    // Bloc.
    bool HasBlocObstacle() const;
    const CrateObstacleData* GetBlocObstacle(const std::string& tagName) const;
    void AddBlocObstacle(const std::string& tagName, CrateObstacleData obstacle);

    // Teleporter.
    bool HasBlocTeleporter() const;
    const BlocTeleporterData* GetBlocTeleporter(const std::string& tagName) const;
    void AddBlocTeleporter(const std::string& tagName, BlocTeleporterData teleporter);

    // Mach.
    bool HasMach() const;
    const MachineData* GetMach(const std::string& tagName) const;
    void AddMach(const std::string& tagName, MachineData machine);

    // Navigation graph.
    const std::vector<AINavigationCluster> GetNavigationGraph() const;
    void SetNavigationGraph(std::vector<AINavigationCluster> clusters);

    // Raw. (to see all the clusters)
    void SetRawNavigationGraph(std::vector<AINavigationCluster> clusters);
    const std::vector<AINavigationCluster> GetRawNavigationGraph() const;

    // ----- Dynamic Data -----
    // 
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
    std::unordered_map<std::string, SceneryObstacleData> m_ScenObstacles;
    std::unordered_map<std::string, CrateObstacleData> m_BlocObstacles;
    std::unordered_map<std::string, BlocTeleporterData> m_BlocTeleporters;
    std::unordered_map<std::string, MachineData> m_Machs;
    std::vector<AINavigationCluster> m_NavigationGraph;
    std::vector<AINavigationCluster> m_RawNavigationGraph;

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