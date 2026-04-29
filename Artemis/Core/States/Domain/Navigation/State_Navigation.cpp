#include "pch.h"
#include "Core/States/Domain/Navigation/State_Navigation.h"

bool State_Navigation::HasGeometry() const 
{ 
	std::lock_guard<std::mutex> lock(m_Mutex);
	return !m_Geometries.empty();
}

int32_t State_Navigation::GetGeometryCount() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return static_cast<int32_t>(m_Geometries.size());
}

const SbspGeometry* State_Navigation::GetGeometry(int32_t index) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (index < 0 || index >= static_cast<int32_t>(m_Geometries.size()))
	{
		return nullptr;
	}

	return &m_Geometries[index];
}

const std::vector<SbspGeometry>& State_Navigation::GetAllGeometry() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Geometries;
}

void State_Navigation::AddGeometry(SbspGeometry geometry) 
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Geometries.push_back(std::move(geometry));
}

void State_Navigation::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Geometries.clear();
}