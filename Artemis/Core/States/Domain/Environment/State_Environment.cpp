#include "pch.h"
#include "Core/States/Domain/Environment/State_Environment.h"

bool State_Environment::HasCollGeometry(const std::string& tagName) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return !m_CollGeometries.empty();
}

const CollGeometry* State_Environment::GetCollGeometry(const std::string& tagName) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_CollGeometries.find(tagName);
	if (it == m_CollGeometries.end())
	{
		return nullptr;
	}
	return &it->second;
}

void State_Environment::AddCollGeometry(const std::string& tagName, CollGeometry geometry)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_CollGeometries[tagName] = std::move(geometry);
}


bool State_Environment::HasPhmoGeometry(const std::string& tagName) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return !m_PhmoGeometries.empty();
}

const PhmoGeometry* State_Environment::GetPhmoGeometry(const std::string& tagName) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_PhmoGeometries.find(tagName);
	if (it == m_PhmoGeometries.end())
	{
		return nullptr;
	}
	return &it->second;
}

void State_Environment::AddPhmoGeometry(const std::string& tagName, PhmoGeometry geometry)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PhmoGeometries[tagName] = std::move(geometry);
}

bool State_Environment::HasModeGeometry(const std::string& tagName) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return !m_ModeGeometries.empty();
}

const ModeGeometry* State_Environment::GetModeGeometry(const std::string& tagName) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_ModeGeometries.find(tagName);
	if (it == m_ModeGeometries.end())
	{
		return nullptr;
	}
	return &it->second;
}

void State_Environment::AddModeGeometry(const std::string& tagName, ModeGeometry geometry)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_ModeGeometries[tagName] = std::move(geometry);
}

bool State_Environment::HasMapZones() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_HasMapZones;
}

const ScnrMapZones* State_Environment::GetMapZones() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_HasMapZones ? &m_MapZones : nullptr;
}

void State_Environment::SetMapZones(ScnrMapZones zones)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_MapZones = std::move(zones);
	m_HasMapZones = true;
}

void State_Environment::Cleanup()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_CollGeometries.clear();
	m_PhmoGeometries.clear();
	m_ModeGeometries.clear();
	m_HasMapZones = false;
	m_MapZones = {};
}