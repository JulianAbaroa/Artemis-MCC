export module Map.Reader.System:Header;

import :Formula;

import Map.Reader.Type;
import std;

export namespace Map::Reader::System
{
    // Reads the map header.
    // note: Offset mask 3 is not used yet. It possibly corresponds to the locales, which would need their own conversion in FormulaService.
    class HeaderService
    {
    private:
        using HeaderInfo = Map::Reader::Type::Info::HeaderInfo;
        using FormulaService = Map::Reader::System::FormulaService;

    public:
        explicit HeaderService(FormulaService& formulaService) :
            m_FormulaService(formulaService) {}
        ~HeaderService() = default;

        // Reads the values the address conversions depend on, and initializes the FormulaService with them.
        // return: Nothing if the header is cut short.
        auto Read(std::ifstream& file) -> std::optional<HeaderInfo>;

    private:
        FormulaService& m_FormulaService;
    };
}