#include "pch.h"

#include "State_Navigation.h"

// --- Static Data ---

// Sbsp Gemtretry.
bool State_Navigation::HasSbspGeometry() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return !m_SbspGeometries.empty();
}

const SbspGeometry* State_Navigation::GetSbspGeometry(int32_t index) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (index < 0 || index >= static_cast<int32_t>(m_SbspGeometries.size()))
    {
        return nullptr;
    }

    return &m_SbspGeometries[index];
}

const std::vector<SbspGeometry>& State_Navigation::GetAllSbspGeometry() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_SbspGeometries;
}

void State_Navigation::AddSbspGeometry(SbspGeometry geometry)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_SbspGeometries.push_back(std::move(geometry));
}

int32_t State_Navigation::GetSbspGeometryCount() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return static_cast<int32_t>(m_SbspGeometries.size());
}

bool State_Navigation::HasMapZones() const
{
    return m_HasMapZones.load();
}

const ScnrMapZones* State_Navigation::GetMapZones() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_HasMapZones.load() ? &m_MapZones : nullptr;
}

void State_Navigation::SetMapZones(ScnrMapZones zones)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MapZones = std::move(zones);
    m_HasMapZones.store(true);
}

const std::vector<AINavigationCluster> State_Navigation::GetNavigationGraph() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_NavigationGraph;
}

void State_Navigation::SetNavigationGraph(std::vector<AINavigationCluster> clusters)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_NavigationGraph = std::move(clusters);
}

// ----- Dynamic Data -----

// Obstacles.
std::vector<ActiveObstacle> State_Navigation::GetActiveObstacles() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ActiveObstacles;
}

void State_Navigation::SetActiveObstacles(std::vector<ActiveObstacle> obstacles)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveObstacles = std::move(obstacles);
}

// Spawns.
std::vector<ActiveSpawn> State_Navigation::GetActiveSpawns() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ActiveSpawns;
}

void State_Navigation::SetActiveSpawns(std::vector<ActiveSpawn> spawns)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveSpawns = std::move(spawns);
}

// Teleports.
std::vector<ActiveTeleporter> State_Navigation::GetActiveTeleporters() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ActiveTeleporters;
}

void State_Navigation::SetActiveTeleporters(std::vector<ActiveTeleporter> teleporters)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveTeleporters = std::move(teleporters);
}

// Lifts.
std::vector<ActiveLift> State_Navigation::GetActiveLifts() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ActiveLifts;
}

void State_Navigation::SetActiveLifts(std::vector<ActiveLift> lifts)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveLifts = std::move(lifts);
}

// Shields.
std::vector<ActiveShield> State_Navigation::GetActiveShields() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ActiveShields;
}

void State_Navigation::SetActiveShields(std::vector<ActiveShield> shields)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveShields = std::move(shields);
}

// Objective Spawns.
std::vector<ActiveObjectiveSpawn> State_Navigation::GetActiveObjectiveSpawns() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ActiveObjectiveSpawns;
}

void State_Navigation::SetActiveObjectiveSpawns(std::vector<ActiveObjectiveSpawn> spawns)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveObjectiveSpawns = std::move(spawns);
}

// Objectives.
std::vector<ActiveObjective> State_Navigation::GetActiveObjectives() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ActiveObjectives;
}

void State_Navigation::SetActiveObjectives(std::vector<ActiveObjective> objectives)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveObjectives = std::move(objectives);
}

// Destructibles.
std::vector<ActiveDestructible> State_Navigation::GetActiveDestructibles() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ActiveDestructibles;
}

void State_Navigation::SetActiveDestructibles(std::vector<ActiveDestructible> destructibles)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ActiveDestructibles = std::move(destructibles);
}

void State_Navigation::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_SbspGeometries.clear();
    m_MapZones = {};
    m_HasMapZones.store(false);

    m_NavigationGraph.clear();
    m_RawNavigationGraph.clear();
    m_ActiveObstacles.clear();
    m_ActiveSpawns.clear();
    m_ActiveTeleporters.clear();
    m_ActiveLifts.clear();
    m_ActiveShields.clear();
    m_ActiveObjectiveSpawns.clear();
    m_ActiveObjectives.clear();
    m_ActiveDestructibles.clear();
}