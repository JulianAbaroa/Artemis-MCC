#pragma once

#include "Core/Types/Sources/MapReader/MapTypes.h"

#include <atomic>
#include <vector>
#include <mutex>

class State_MapReader
{
public:
	bool IsLoaded() const;
	void SetLoaded(bool value);

	// --- Map file paths ---
	std::string GetMapFilePath();
	void SetMapFilePath(std::string path);

	std::string GetCampaignFilePath() const;
	void SetCampaignFilePath(std::string path);

	std::string GetSharedFilePath() const;
	void SetSharedFilePath(std::string path);

	// --- Map Data ---
	const Map_TagTableEntry& GetTag(int32_t index) const;
	uint32_t GetGroupMagic(int16_t groupIndex) const;
	std::string GetTagName(int32_t index) const;

	Map_TagTableEntry* GetTagsData();
	Map_TagTableGroupEntry* GetGroupsData();
	int32_t* GetNameOffsetsData();
	char* GetNameData();

	size_t GetTagsSize() const;
	size_t GetGroupsSize() const;
	size_t GetNameOffsetsSize() const;

	void ResizeTags(int32_t count);
	void ResizeGroups(int32_t count);
	void ResizeNameData(int32_t count);
	void ResizeNameOffsets(int32_t count);

	void Cleanup();

private:
	std::atomic<bool> m_IsLoaded{ false };

	std::string m_MapFilePath;
	std::string m_CampaingFilePath;
	std::string m_SharedFilePath;

	std::vector<Map_TagTableEntry> m_Tags;
	std::vector<Map_TagTableGroupEntry> m_Groups;
	std::vector<int32_t> m_NameOffsets;
	std::vector<char> m_NameData;

	mutable std::mutex m_Mutex;
};