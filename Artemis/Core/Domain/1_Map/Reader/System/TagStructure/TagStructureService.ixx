export module Map.Reader.System:TagStructure;

import Map.Reader.Type;
import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
	class TagStructureService
	{
	private:
		using TagBlock = Map::Reader::Type::Structure::Tag::Block;
		using TagTableEntry = Map::Reader::Type::Structure::TagTable::Entry;

		using TagIndexStore = Map::Reader::State::TagIndexStore;

	public:
		explicit TagStructureService(TagIndexStore& m_TagIndexStore) :
			m_TagIndexStore(m_TagIndexStore) {}
		~TagStructureService() = default;

		auto ReadTagReference(std::ifstream& file, 
			std::int64_t tagRefOffset) const -> TagTableEntry;

		auto ReadTagBlock(std::ifstream& file, 
			std::int64_t blockHeaderOffset) const -> TagBlock;

	private:
		TagIndexStore& m_TagIndexStore;
	};
}