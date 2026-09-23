export module Map.Reader.System:Header;

import :Formula;
import Map.Reader.Type;
import std;

export namespace Map::Reader::System
{
	class HeaderService
	{
	private:
		using HeaderInfo = Map::Reader::Type::Info::HeaderInfo;
		using FormulaService = Map::Reader::System::FormulaService;

	public:
		explicit HeaderService(FormulaService& m_FormulaService) :
			m_FormulaService(m_FormulaService) {}
		~HeaderService() = default;

		auto Read(std::ifstream& file) -> std::optional<HeaderInfo>;

	private:
		FormulaService& m_FormulaService;
	};
}