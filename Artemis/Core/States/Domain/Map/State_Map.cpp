#include "pch.h"
#include "Core/States/Domain/Map/State_Map.h"

bool State_Map::IsLoaded() const { return m_IsLoaded.load(); }
void State_Map::SetLoaded(bool value) { m_IsLoaded.store(value); }

std::string State_Map::GetMapFilePath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_MapFilePath;
}

void State_Map::SetMapFilePath(std::string path)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_MapFilePath = path;
}

const Map_TagTableEntry& State_Map::GetTag(int32_t index) const
{
	return m_Tags.at(index);
}

uint32_t State_Map::GetGroupMagic(int16_t groupIndex) const
{
	if (groupIndex < 0 || groupIndex >= m_Groups.size()) return 0;
	return m_Groups[groupIndex].Magic;
}

std::string State_Map::GetTagName(int32_t index) const
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

Map_TagTableGroupEntry* State_Map::GetGroupsData() { return m_Groups.data(); }
Map_TagTableEntry* State_Map::GetTagsData() { return m_Tags.data(); }
int32_t* State_Map::GetNameOffsetsData() { return m_NameOffsets.data(); }
char* State_Map::GetNameData() { return m_NameData.data(); }

size_t State_Map::GetTagsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Tags.size();
}

size_t State_Map::GetGroupsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Groups.size();
}

size_t State_Map::GetNameOffsetsSize() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_NameOffsets.size();
}

void State_Map::ResizeTags(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Tags.resize(count);
}

void State_Map::ResizeGroups(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Groups.resize(count);
}

void State_Map::ResizeNameOffsets(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_NameOffsets.resize(count);
}

void State_Map::ResizeNameData(int32_t count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_NameData.resize(count);
}

void State_Map::Cleanup()
{
	m_IsLoaded.store(false);

	std::lock_guard<std::mutex> lock(m_Mutex);

	m_Tags.clear();
	m_Groups.clear();
	m_NameData.clear();
	m_NameOffsets.clear();
}