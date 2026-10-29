export module Blam.Map.Reader.System:TagStructure;

import Blam.Map.Reader.Type;
import Blam.Map.Reader.State;
import std;

export namespace Blam::Map::Reader::System
{
    // Reads the structures that tags embed: references to other tags and blocks.
    class TagStructureService
    {
    private:
        using TagBlock = Blam::Map::Reader::Type::Structure::Tag::TagBlock;
        using TagTableEntry = Blam::Map::Reader::Type::Structure::TagTable::TagTableEntry;

        using TagIndexStore = Blam::Map::Reader::State::TagIndexStore;

    public:
        explicit TagStructureService(TagIndexStore& tagIndexStore) :
            m_TagIndexStore(tagIndexStore) {}
        ~TagStructureService() = default;

        // Reads a tag reference and looks up the tag it points to.
        // param tagRefOffset: File offset of the reference.
        // return: Empty entry if the offset is invalid or the reference is null or points outside the tag table.
        auto ReadTagReference(std::ifstream& file,
            std::int64_t tagRefOffset) const -> TagTableEntry;

        // Reads the header of a block.
        // param blockHeaderOffset: File offset of the block header.
        // return: Empty block if the offset is invalid or the header is cut short.
        auto ReadTagBlock(std::ifstream& file,
            std::int64_t blockHeaderOffset) const -> TagBlock;

    private:
        TagIndexStore& m_TagIndexStore;
    };
}