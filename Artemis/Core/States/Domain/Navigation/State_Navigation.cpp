#include "pch.h"

// Header.
#include "State_Navigation.h"

// ----- Static Data -----

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

// Scen.
bool State_Navigation::HasScenObstacle() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return !m_ScenObstacles.empty();
}

const SceneryObstacleData* State_Navigation::GetScenObstacle(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_ScenObstacles.find(tagName);
    return it != m_ScenObstacles.end() ? &it->second : nullptr;
}

void State_Navigation::AddScenObstacle(const std::string& tagName, SceneryObstacleData obstacle)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ScenObstacles[tagName] = std::move(obstacle);
}

// Bloc.
bool State_Navigation::HasBlocObstacle() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return !m_BlocObstacles.empty();
}

const CrateObstacleData* State_Navigation::GetBlocObstacle(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_BlocObstacles.find(tagName);
    return it != m_BlocObstacles.end() ? &it->second : nullptr;
}

void State_Navigation::AddBlocObstacle(const std::string& tagName,
    CrateObstacleData obstacle)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_BlocObstacles[tagName] = std::move(obstacle);
}

// Teleporter.
bool State_Navigation::HasBlocTeleporter() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return !m_BlocTeleporters.empty();
}

const BlocTeleporterData* State_Navigation::GetBlocTeleporter(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_BlocTeleporters.find(tagName);
    return it != m_BlocTeleporters.end() ? &it->second : nullptr;
}

void State_Navigation::AddBlocTeleporter(const std::string& tagName, BlocTeleporterData teleporter)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_BlocTeleporters[tagName] = std::move(teleporter);
}

// Mach.
bool State_Navigation::HasMach() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return !m_Machs.empty();
}

const MachineData* State_Navigation::GetMach(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Machs.find(tagName);
    return it != m_Machs.end() ? &it->second : nullptr;
}

void State_Navigation::AddMach(const std::string& tagName, MachineData machine)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Machs[tagName] = std::move(machine);
}

// Navigation graph.
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

// Raw.
const std::vector<AINavigationCluster> State_Navigation::GetRawNavigationGraph() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_RawNavigationGraph;
}

// ----- Dynamic Data -----

void State_Navigation::SetRawNavigationGraph(std::vector<AINavigationCluster> clusters)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_RawNavigationGraph = std::move(clusters);
}


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

    // Static data.
    m_SbspGeometries.clear();
    m_ScenObstacles.clear();
    m_BlocObstacles.clear();
    m_BlocTeleporters.clear();
    m_Machs.clear();

    // Dynamic data.
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