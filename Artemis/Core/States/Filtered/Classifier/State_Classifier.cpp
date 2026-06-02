#include "pch.h"

#include "State_Classifier.h"

const std::vector<Classified>& State_Classifier::GetClassifieds() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Objects;
}

void State_Classifier::SetClassifieds(std::vector<Classified> classifieds)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Objects = std::move(classifieds);
}

void State_Classifier::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Objects.clear();
}