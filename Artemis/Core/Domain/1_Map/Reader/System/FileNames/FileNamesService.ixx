export module Map.Reader.System:FileNames;

import :Formula;

import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
    // Reads the table of tag names of the map.
    class FileNamesService
    {
    private:
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using FormulaService = Map::Reader::System::FormulaService;

    public:
        FileNamesService(TagIndexStore& tagIndexStore,
            FormulaService& formulaService) :
            m_TagIndexStore(tagIndexStore),
            m_FormulaService(formulaService) {}
        ~FileNamesService() = default;

        // Reads the name offsets and the name characters into the tag index store.
        // param fileIndexTableOffset: Pointer to the table of name offsets.
        // param fileTableOffset: Pointer to the name characters.
        // param fileTableSize: Size in bytes of the name characters.
        // return: False if a table is cut short. True if there are no names to read.
        auto Read(std::ifstream& file, std::int32_t fileTableCount,
            std::int64_t fileIndexTableOffset, std::int64_t fileTableOffset,
            std::int32_t fileTableSize) const -> bool;

    private:
        TagIndexStore& m_TagIndexStore;
        FormulaService& m_FormulaService;
    };
}