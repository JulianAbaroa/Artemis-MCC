export module Map.Reader.System:TagReference;

import :Formula;

import Map.Reader.Type;
import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
    // Turns the values stored in the map into offsets and names: where a tag and its blocks are, and which tag a reference points to.
    class TagReferenceService
    {
    private:
        using TagTableEntry = Map::Reader::Type::Structure::TagTable::TagTableEntry;
        using TagReference = Map::Reader::Type::Structure::Tag::TagReference;
        using TagBlock = Map::Reader::Type::Structure::Tag::TagBlock;

        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using FormulaService = Map::Reader::System::FormulaService;

    public:
        TagReferenceService(TagIndexStore& tagIndexStore, FormulaService& formulaService) :
            m_TagIndexStore(tagIndexStore), m_FormulaService(formulaService) {}
        ~TagReferenceService() = default;

        // return: File offset of the tag data, or -1 if the index is out of range or the tag has no data.
        auto GetTagOffset(std::int32_t tagIndex) const -> std::int64_t;

        // return: File offset of the tag data, or -1 if the tag has no data.
        auto ResolveTagOffset(const TagTableEntry& tag) const -> std::int64_t;

        // return: Name of the tag the reference points to, or empty if the reference is null or points outside the tag table.
        auto ResolveTagReferenceName(const TagReference& reference) const -> std::string;

        // return: File offset of the first entry of the block, or -1 if the block is empty.
        auto ResolveBlockOffset(const TagBlock& block) const -> std::int64_t;

    private:
        TagIndexStore& m_TagIndexStore;
        FormulaService& m_FormulaService;
    };
}