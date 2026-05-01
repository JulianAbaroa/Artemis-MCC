#include "pch.h"
#include "Core/States/Domain/Navigation/State_Navigation.h"

bool State_Navigation::HasSbspGeometry() const 
{ 
	std::lock_guard<std::mutex> lock(m_Mutex);
	return !m_SbspGeometries.empty();
}

int32_t State_Navigation::GetSbspGeometryCount() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return static_cast<int32_t>(m_SbspGeometries.size());
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

void State_Navigation::AddBlocObstacle(const std::string& tagName, CrateObstacleData obstacle)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_BlocObstacles[tagName] = std::move(obstacle);
}

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

void State_Navigation::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_SbspGeometries.clear();
	m_ScenObstacles.clear();
	m_BlocObstacles.clear();
	m_BlocTeleporters.clear();
	m_Machs.clear();
}