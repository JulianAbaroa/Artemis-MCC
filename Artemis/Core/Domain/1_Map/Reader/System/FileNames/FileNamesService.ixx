export module Map.Reader.System:FileNames;

import :Formula;
import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
	class FileNamesService
	{
	private:
		using TagIndexStore = Map::Reader::State::TagIndexStore;
		using FormulaService = Map::Reader::System::FormulaService;

	public:
		FileNamesService(TagIndexStore& m_TagIndexStore,
			FormulaService& m_FormulaService) :
			m_TagIndexStore(m_TagIndexStore),
			m_FormulaService(m_FormulaService) {}
		~FileNamesService() = default;

		auto Read(std::ifstream& file, std::int32_t fileTableCount,
			std::int64_t fileIndexTableOffset, std::int64_t fileTableOffset,
			std::int32_t fileTableSize) const -> bool;

	private:
		TagIndexStore& m_TagIndexStore;
		FormulaService& m_FormulaService;
	};
}