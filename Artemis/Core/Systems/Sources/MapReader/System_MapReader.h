#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>

struct ResolvedTag;
struct Map_TagRef;
struct Map_TagBlock;
struct Map_TagTableEntry;

class State_MapReader;
class System_Logs;

struct Sys_MapReader_Deps
{
	State_MapReader& State_MapReader;
	System_Logs& System_Logs;
};

class System_MapReader
{
public:
	System_MapReader(Sys_MapReader_Deps deps) : m_Deps(deps) {}
	~System_MapReader() = default;

	void LoadMap(std::string& filePath);

	ResolvedTag ResolveHandle(uint32_t handle) const;
	int64_t GetTagOffset(int32_t tagIndex) const;

	FILE* OpenMapFile(const std::string& filePath) const;
	
	std::string ResolveExternalCachePath(
		const std::string& mapPath) const;

	Map_TagTableEntry ReadTagRef(FILE* file, int64_t tagRefOffset) const;
	std::string ResolveTagRefName(const Map_TagRef& ref) const;

	int64_t ResolveTagOffset(const Map_TagTableEntry& tag) const;

	Map_TagBlock ReadTagBlock(FILE* file, int64_t blockHeaderOffset) const;
	int64_t ResolveBlockOffset(const Map_TagBlock& block) const;

	std::vector<uint8_t> ReadData(int64_t fileOffset, int32_t size) const;

	std::vector<uint8_t> ReadSegment(int64_t fileOffset,
		int32_t compressedSize, int32_t decompressedSize, 
		int32_t segmentOffset, int32_t segmentLength) const;

	std::vector<uint8_t> ReadDataFrom(
		const std::string& filePath, int64_t fileOffset, 
		int32_t size) const;

	std::vector<uint8_t> ReadSegmentFrom(
		const std::string& filePath, int64_t fileOffset, 
		int32_t compressedSize, int32_t decompressedSize,
		int32_t segmentOffset, int32_t segmentLength) const;

	int64_t Expand(uint32_t address) const;
	int64_t ToFileOffset(int64_t virtualAddress) const;
	int64_t ToDebugOffset(int64_t pointer) const;
	int64_t ToResourceOffset(int64_t blockOffset) const;

	void Cleanup();

private:
	Sys_MapReader_Deps m_Deps;

	uint64_t m_VirtualBaseAddress = 0;
	int64_t m_IndexHeaderFileOffset = 0;
	int32_t m_FileTableCount = 0;
	int32_t m_FileTableOffset = 0;
	int32_t m_FileTableSize = 0;
	int32_t m_FileIndexTableOffset = 0;
	uint32_t m_DebugOffsetMask = 0;
	uint32_t m_ResourceOffsetMask = 0;
	uint32_t m_TagOffsetMask = 0;
	uint32_t m_DebugVirtualAddress = 0;
	uint32_t m_ResourceVirtualAddress = 0;
	uint32_t m_TagVirtualAddress = 0;
	int64_t m_TagSectionFileOffset = 0;

	static constexpr int64_t  k_ExpandMagic = 0x50000000LL;

	bool ReadHeader(FILE* file);
	bool ReadIndexHeader(FILE* file);
	bool ReadTagGroups(FILE* file, int64_t fileOffset, int32_t count);
	bool ReadTags(FILE* file, int64_t fileOffset, int32_t count);
	bool ReadFileNames(FILE* file) const;

	std::string MagicToString(int32_t magic) const;
};