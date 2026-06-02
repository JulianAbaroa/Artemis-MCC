#include "pch.h"

#include "State_MapReader.h"

bool State_MapReader::IsLoaded() const 
{ 
	return m_IsLoaded.load(); 
}

void State_MapReader::SetLoaded(bool value)
{ 
	m_IsLoaded.store(value); 
}

std::string State_MapReader::GetMapFilePath()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_MapFilePath;
}

void State_MapReader::SetMapFilePath(std::string path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_MapFilePath = path;
}

std::string State_MapReader::GetCampaignFilePath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_CampaingFilePath;
}

void State_MapReader::SetCampaignFilePath(std::string path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_CampaingFilePath = path;
}


std::string State_MapReader::GetSharedFilePath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_SharedFilePath;
}

void State_MapReader::SetSharedFilePath(std::string path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_SharedFilePath = path;
}


const Map_TagTableEntry& State_MapReader::GetTag(int32_t index) const
{
	return m_Tags.at(index);
}

uint32_t State_MapReader::GetGroupMagic(int16_t groupIndex) const
{
	if (groupIndex < 0 || groupIndex >= m_Groups.size()) return 0;
	return m_Groups[groupIndex].Magic;
}

std::string State_MapReader::GetTagName(int32_t index) const
{
	if (index < 0 || index >= (int32_t)m_NameOffsets.size())
	{
		return "";
	}
	
	int32_t offset = m_NameOffsets[index];
	if (offset == -1 || offset >= (int32_t)m_NameData.size())
	{
		return "unknown";
	}

	return std::string(&m_NameData[offset]);
}

Map_TagTableGroupEntry* State_MapReader::GetGroupsData()
{ 
	return m_Groups.data(); 
}

Map_TagTableEntry* State_MapReader::GetTagsData()
{ 
	return m_Tags.data(); 
}

int32_t* State_MapReader::GetNameOffsetsData()
{ 
	return m_NameOffsets.data(); 
}

char* State_MapReader::GetNameData()
{ 
	return m_NameData.data(); 
}

size_t State_MapReader::GetTagsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Tags.size();
}

size_t State_MapReader::GetGroupsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Groups.size();
}

size_t State_MapReader::GetNameOffsetsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_NameOffsets.size();
}

void State_MapReader::ResizeTags(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Tags.resize(count);
}

void State_MapReader::ResizeGroups(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Groups.resize(count);
}

void State_MapReader::ResizeNameOffsets(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_NameOffsets.resize(count);
}

void State_MapReader::ResizeNameData(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_NameData.resize(count);
}

void State_MapReader::Cleanup()
{
	m_IsLoaded.store(false);

	std::lock_guard<std::mutex> lock(m_Mutex);

	m_Tags.clear();
	m_Groups.clear();
	m_NameData.clear();
	m_NameOffsets.clear();
	m_MapFilePath.clear();
}