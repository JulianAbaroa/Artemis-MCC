#pragma once

#include "../System_MapReader.h"

class System_MapReader;

template <typename TObject>
struct GroupDescriptor;

class TagGroupReader
{
public:
    explicit TagGroupReader(System_MapReader& map) : m_Map(map) {}

    template <typename TObject>
    TObject Read(FILE* file, int64_t tagOffset, const std::string& tagName) const
    {
        using TData = typename GroupDescriptor<TObject>::DataType;

        TObject object{};
        object.TagName = tagName;
        if (tagOffset < 0) return object;

        if (fseek(file, (long)tagOffset, SEEK_SET) != 0)
        {
            return object;
        }

        if (fread(&object.Data, sizeof(TData), 1, file) != 1)
        {
            return object;
        }

        GroupDescriptor<TObject>::ReadBlocks(file, *this, object);
        return object;
    }

    template <typename TEntry>
    std::vector<TEntry> ReadBlock(FILE* file, const Map_TagBlock& block) const
    {
        std::vector<TEntry> result;
        int64_t offset = m_Map.ResolveBlockOffset(block);
        if (offset < 0) return result;

        result.resize(block.EntryCount);
        if (fseek(file, (long)offset, SEEK_SET) != 0)
        {
            return result;
        }

        fread(result.data(), sizeof(TEntry), (size_t)block.EntryCount, file);
        return result;
    }

private:
    System_MapReader& m_Map;
};