#pragma once

#include "Core/Types/Domain/Domains/Navigation/SbspGeometry.h"
#include <vector>
#include <mutex>

class State_Navigation
{
public:
	bool HasGeometry() const;
	int32_t GetGeometryCount() const;
	const SbspGeometry* GetGeometry(int32_t index) const;
	const std::vector<SbspGeometry>& GetAllGeometry() const;
	void AddGeometry(SbspGeometry geometry);
	
	void Cleanup();

private:
	std::vector<SbspGeometry> m_Geometries;
	mutable std::mutex m_Mutex;
};