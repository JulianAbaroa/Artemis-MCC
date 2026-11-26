module DevTools.Workbench.Skip.System;

namespace DevTools::Workbench::Skip::System
{
    auto DecideSkip(std::string_view text, std::string_view scenarioId, std::size_t matchNumber) -> Type::SkipDecision
    {
        std::istringstream input{ std::string{ text } };
        std::string id{};
        std::string number{};
        std::string extra{};

        if (!(input >> id >> number) || (input >> extra)) return Type::SkipDecision::Invalid;

        std::size_t value{ 0 };
        const char* end{ number.data() + number.size() };
        const auto [stop, error]{ std::from_chars(number.data(), end, value) };

        if (error != std::errc{} || stop != end || value == 0) return Type::SkipDecision::Invalid;

        return id == scenarioId && value == matchNumber ? Type::SkipDecision::Apply : Type::SkipDecision::Stale;
    }
}