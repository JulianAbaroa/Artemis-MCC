#pragma once

#include "Core/Types/Environment/EnvironmentTypes.h"
#include "Core/Types/Environment/CollGeometry.h"
#include "Core/Types/Environment/PhmoGeometry.h"
#include "Core/Types/Environment/ModeGeometry.h"

#include <unordered_map>
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

	// --- Dynamic Data ---

	const std::vector<ActivePhysicsInstance> GetActivePhysicsInstances() const;
	void SetActivePhysicsInstances(std::vector<ActivePhysicsInstance> instances);

	void Cleanup();

private:
	// --- Static Data ---

	std::unordered_map<std::string, CollGeometry> m_CollGeometries;
	std::unordered_map<std::string, PhmoGeometry> m_PhmoGeometries;
	std::unordered_map<std::string, ModeGeometry> m_ModeGeometries;

	// --- Dynamic Data ---

	std::vector<ActivePhysicsInstance> m_PhysicsInstances;

	mutable std::mutex m_Mutex;
};