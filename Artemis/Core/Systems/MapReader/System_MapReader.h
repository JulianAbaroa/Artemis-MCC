#pragma once

#include <string>
#include <vector>

struct ResolvedTag;
struct Map_TagBlock;
struct Map_TagTableEntry;

class State_MapReader;
class System_Logs;

struct System_MapReader_Dependencies
{
	State_MapReader& State_Map;
	System_Logs& System_Logs;
};

class System_MapReader
{
public:
	System_MapReader(System_MapReader_Dependencies depedencies) :
		m_Deps(depedencies) {}
	~System_MapReader() = default;

	void LoadMap(std::string& path);

	void Cleanup();

	// --- Tag Table Queries ---

	ResolvedTag ResolveHandle(uint32_t handle) const;
	int64_t GetTagMetaOffset(const ResolvedTag& tag) const;
	int64_t GetTagMetaOffsetByIndex(int32_t tagIndex) const;

	// --- Shared Parsing Infrastructure ---

	FILE* OpenMapFile() const;

	Map_TagTableEntry ReadTagRef(FILE* file, int64_t tagRefOffset) const;
	int64_t ResolveTagOffset(const Map_TagTableEntry& tag) const;

	Map_TagBlock ReadTagBlock(FILE* file, int64_t blockHeaderOffset) const;
	int64_t ResolveBlockOffset(const Map_TagBlock& block) const;

	std::vector<uint8_t> ReadResourceData(int64_t fileOffset, int32_t size) const;

	std::vector<uint8_t> ReadResourceDataDecompressed(int64_t fileOffset,
		int32_t compressedSize, int32_t decompressedSize,
		int32_t segmentOffset, int32_t segmentLength) const;

	std::vector<uint8_t> ReadDataFrom(const std::string& filePath,
		int64_t fileOffset, int32_t size) const;

	std::string ResolveExternalCachePath(
		const std::string& mapPath) const;

	std::vector<uint8_t> ReadResourceDataDecompressedFrom(
		const std::string& filePath, int64_t fileOffset, 
		int32_t compressedSize, int32_t decompressedSize,
		int32_t segmentOffset, int32_t segmentLength) const;

	// --- Address Arithmetic ---

	int64_t Expand(uint32_t address) const;
	int64_t ToFileOffset(int64_t virtualAddress) const;
	int64_t ToDebugOffset(int64_t pointer) const;
	int64_t ToResourceOffset(int64_t blockOffset) const;

private:
	System_MapReader_Dependencies m_Deps;

	// --- .MAP Parsing ---

	bool ReadHeader(FILE* file);
	bool ReadIndexHeader(FILE* file);
	bool ReadTagGroups(FILE* file, int64_t fileOffset, int32_t count);
	bool ReadTags(FILE* file, int64_t fileOffset, int32_t count);
	bool ReadFileNames(FILE* file) const;

	std::string MagicToString(int32_t magic) const;

	// --- Cached Header Values ---

	uint32_t m_DebugOffsetMask = 0;
	uint32_t m_ResourceOffsetMask = 0;
	uint32_t m_TagOffsetMask = 0;
	uint32_t m_DebugVirtualAddress = 0;
	uint32_t m_ResourceVirtualAddress = 0;
	uint32_t m_TagVirtualAddress = 0;
	int32_t  m_FileTableCount = 0;
	int32_t  m_FileTableOffset = 0;
	int32_t  m_FileTableSize = 0;
	int32_t  m_FileIndexTableOffset = 0;
	uint64_t m_VirtualBaseAddress = 0;
	int64_t  m_MetaFileOffset = 0;
	int64_t  m_IndexHeaderFileOffset = 0;

	static constexpr int64_t  k_ExpandMagic = 0x50000000LL;
};