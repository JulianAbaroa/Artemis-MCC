#pragma once

#include "Core/Types/Domain/Domains/Environment/CollGeometry.h"
#include "Core/Types/Domain/Domains/Environment/PhmoGeometry.h"
#include "Core/Types/Domain/Domains/Environment/ModeGeometry.h"
#include <unordered_map>
#include <mutex>

class State_Environment
{
public:
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

	void Cleanup();

private:
	std::unordered_map<std::string, CollGeometry> m_CollGeometries;
	std::unordered_map<std::string, PhmoGeometry> m_PhmoGeometries;
	std::unordered_map<std::string, ModeGeometry> m_ModeGeometries;
	mutable std::mutex m_Mutex;
};