#pragma once

// Types.
#include "Core/Types/Domain/Environment/CollGeometry.h"
#include "Core/Types/Domain/Environment/PhmoGeometry.h"
#include "Core/Types/Domain/Environment/ModeGeometry.h"
#include "Core/Types/Domain/Environment/ScnrZones.h"
#include "Core/Types/Domain/Environment/BipdPhysicsData.h"
#include "Core/Types/Domain/Environment/ScenZoneData.h"
#include "Core/Types/Domain/Environment/EnvironmentTypes.h"

#include <unordered_map>
#include <atomic>
#include <mutex>

class State_Environment
{
public:
	// --- Static Data ---

	// Coll.
	bool HasCollGeometry(const std::string& tagName) const;
	const CollGeometry* GetCollGeometry(const std::string& tagName) const;
	void AddCollGeometry(const std::string& tagName, CollGeometry geometry);

	// Phmo.
	bool HasPhmoGeometry(const std::string& tagName) const;
	const PhmoGeometry* GetPhmoGeometry(const std::string& tagName) const;
	void AddPhmoGeometry(const std::string& tagName, PhmoGeometry geometry);

	// Mode.
	bool HasModeGeometry(const std::string& tagName) const;
	const ModeGeometry* GetModeGeometry(const std::string& tagName) const;
	void AddModeGeometry(const std::string& tagName, ModeGeometry geometry);

	// Scnr.
	bool HasMapZones() const;
	const ScnrMapZones* GetMapZones() const;
	void SetMapZones(ScnrMapZones zones);

	// Bipd.
	bool HasBipdData(const std::string& tagName) const;
	const BipdPhysicsData* GetBipdData(const std::string& tagName) const;
	void AddBipdData(const std::string& tagName, BipdPhysicsData data);

	// Scen.
	bool HasScenData(const std::string& tagName) const;
	const SceneryZoneData* GetScenData(const std::string& tagName) const;
	void AddScenData(const std::string& tagName, SceneryZoneData data);

	// --- Dynamic Data ---

	const std::vector<ActivePhysicsInstance> GetActivePhysicsInstances() const;
	void SetActivePhysicsInstances(std::vector<ActivePhysicsInstance> instances);

	void Cleanup();

private:
	// --- Static Data ---

	std::unordered_map<std::string, CollGeometry> m_CollGeometries;
	std::unordered_map<std::string, PhmoGeometry> m_PhmoGeometries;
	std::unordered_map<std::string, ModeGeometry> m_ModeGeometries;
	std::unordered_map<std::string, BipdPhysicsData> m_BipdData;
	std::unordered_map<std::string, SceneryZoneData> m_ScenData;
	
	ScnrMapZones m_MapZones;
	std::atomic<bool> m_HasMapZones;

	// --- Dynamic Data ---

	std::vector<ActivePhysicsInstance> m_PhysicsInstances;

	mutable std::mutex m_Mutex;
};