export module Map.Reader.System:TagIndex;

import :Formula;

import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
    // Reads the tag index of the map: its groups and its tags.
    class TagIndexService
    {
    private:
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using FormulaService = Map::Reader::System::FormulaService;

    public:
        TagIndexService(TagIndexStore& tagIndexStore, FormulaService& formulaService) :
            m_TagIndexStore(tagIndexStore), m_FormulaService(formulaService) {}
        ~TagIndexService() = default;

        // Reads the groups and the tags into the tag index store.
        // param indexHeaderFileOffset: File offset of the index table header.
        // return: False if the header is not valid or a table is missing or cut short.
        auto Read(std::ifstream& file, std::int64_t indexHeaderFileOffset) -> bool;

    private:
        TagIndexStore& m_TagIndexStore;
        FormulaService& m_FormulaService;

        auto ReadTagGroups(std::ifstream& file, std::int64_t fileOffset,
            std::int32_t count) -> bool;

        auto ReadTags(std::ifstream& file, std::int64_t fileOffset,
            std::int32_t count) -> bool;
    };
}