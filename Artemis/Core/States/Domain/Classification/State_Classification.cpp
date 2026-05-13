#include "pch.h"

// Header.
#include "State_Classification.h"

const std::vector<ClassifiedObject>& State_Classification::GetObjects() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Objects;
}

void State_Classification::SetObjects(std::vector<ClassifiedObject> objects)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Objects = std::move(objects);
}

void State_Classification::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Objects.clear();
}