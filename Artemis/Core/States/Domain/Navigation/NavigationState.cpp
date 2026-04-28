#include "pch.h"
#include "Core/States/Domain/Navigation/NavigationState.h"

bool NavigationState::HasGeometry() const 
{ 
	std::lock_guard<std::mutex> lock(m_Mutex);
	return !m_Geometries.empty();
}

int32_t NavigationState::GetGeometryCount() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return static_cast<int32_t>(m_Geometries.size());
}

const SbspGeometry* NavigationState::GetGeometry(int32_t index) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (index < 0 || index >= static_cast<int32_t>(m_Geometries.size()))
	{
		return nullptr;
	}

	return &m_Geometries[index];
}

const std::vector<SbspGeometry>& NavigationState::GetAllGeometry() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Geometries;
}

void NavigationState::AddGeometry(SbspGeometry geometry) 
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Geometries.push_back(std::move(geometry));
}

void NavigationState::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Geometries.clear();
}