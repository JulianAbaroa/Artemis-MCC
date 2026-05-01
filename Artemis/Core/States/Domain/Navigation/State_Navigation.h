#pragma once

#include "Core/Types/Domain/Domains/Navigation/SbspGeometry.h"
#include "Core/Types/Domain/Domains/Navigation/ScenObstacleData.h"
#include "Core/Types/Domain/Domains/Navigation/BlocObstacleData.h"
#include "Core/Types/Domain/Domains/Navigation/BlocTeleporterData.h"
#include "Core/Types/Domain/Domains/Navigation/MachData.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>

class State_Navigation
{
public:
	// Sbsp.
	bool HasSbspGeometry() const;
	int32_t GetSbspGeometryCount() const;
	const SbspGeometry* GetSbspGeometry(int32_t index) const;
	const std::vector<SbspGeometry>& GetAllSbspGeometry() const;
	void AddSbspGeometry(SbspGeometry geometry);

	// Scen.
	bool HasScenObstacle() const;
	const SceneryObstacleData* GetScenObstacle(const std::string& tagName) const;
	void AddScenObstacle(const std::string& tagName, SceneryObstacleData obstacle);

	// Bloc.
	bool HasBlocObstacle() const;
	const CrateObstacleData* GetBlocObstacle(const std::string& tagName) const;
	void AddBlocObstacle(const std::string& tagName, CrateObstacleData obstacle);

	bool HasBlocTeleporter() const;
	const BlocTeleporterData* GetBlocTeleporter(const std::string& tagName) const;
	void AddBlocTeleporter(const std::string& tagName, BlocTeleporterData teleporter);

	// Mach.
	bool HasMach() const;
	const MachineData* GetMach(const std::string& tagName) const;
	void AddMach(const std::string& tagName, MachineData machine);

	void Cleanup();

private:
	std::unordered_map<std::string, SceneryObstacleData> m_ScenObstacles;
	std::unordered_map<std::string, CrateObstacleData> m_BlocObstacles;
	std::unordered_map<std::string, BlocTeleporterData> m_BlocTeleporters;
	std::unordered_map<std::string, MachineData> m_Machs;
	std::vector<SbspGeometry> m_SbspGeometries;
	mutable std::mutex m_Mutex;
};