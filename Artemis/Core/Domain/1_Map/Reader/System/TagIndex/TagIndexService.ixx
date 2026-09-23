export module Map.Reader.System:TagIndex;

import :Formula;
import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
	class TagIndexService
	{
	private:
		using TagIndexStore = Map::Reader::State::TagIndexStore;
		using FormulaService = Map::Reader::System::FormulaService;

	public:
		TagIndexService(TagIndexStore& m_TagIndexStore, FormulaService& m_FormulaService) :
			m_TagIndexStore(m_TagIndexStore), m_FormulaService(m_FormulaService) {}
		~TagIndexService() = default;

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