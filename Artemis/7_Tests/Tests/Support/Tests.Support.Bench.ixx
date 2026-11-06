export module Tests.Support.Bench;

import std;

// What the offline benches share (F3.B, S2.B): the clock, the statistics of a series of measures and the report file. A bench gathers
// its measures in Series, builds the lines of its report and writes them with WriteReport.

export namespace Tests::Support::Bench
{
    inline constexpr double k_NanosPerMicro{ 1000.0 };

    // return: Microseconds between two instants.
    auto Micros(std::chrono::steady_clock::time_point from, std::chrono::steady_clock::time_point to) -> double
    {
        return std::chrono::duration<double, std::micro>(to - from).count();
    }

    struct Stats
    {
        std::size_t N{};
        double Mean{};
        double P50{};
        double P99{};
        double Max{};
    };

    // Values of one metric, one per measure. Microseconds for times.
    class Series
    {
    public:
        auto Add(double value) -> void { m_Values.push_back(value); }

        auto Summary() const -> Stats
        {
            Stats stats{};
            stats.N = m_Values.size();
            if (m_Values.empty()) return stats;

            std::vector<double> sorted{ m_Values };
            std::ranges::sort(sorted);
            const auto at{ [&](double fraction) -> double
            {
                const std::size_t index{ static_cast<std::size_t>(std::ceil(fraction * static_cast<double>(sorted.size()))) };
                return sorted[(std::min)((std::max)(index, std::size_t{ 1 }), sorted.size()) - 1];
            } };

            stats.Mean = std::accumulate(sorted.begin(), sorted.end(), 0.0) / static_cast<double>(sorted.size());
            stats.P50 = at(0.50);
            stats.P99 = at(0.99);
            stats.Max = sorted.back();
            return stats;
        }

    private:
        std::vector<double> m_Values{};
    };

    // return: The build configuration for the header of a report. Debug numbers are not comparable.
    constexpr auto Configuration() -> std::string_view
    {
#ifdef NDEBUG
        return "Release|x64";
#else
        return "Debug|x64 (numbers are not comparable)";
#endif
    }

    // Writes the lines to build/bench/<fileName> of the repository (outside git), creating the folder.
    // return: The file, or an empty path if it cannot be written.
    auto WriteReport(std::string_view fileName, const std::vector<std::string>& lines) -> std::filesystem::path
    {
        // This file is Artemis/7_Tests/Tests/Support/Tests.Support.Bench.ixx.
        const std::filesystem::path source{ std::source_location::current().file_name() };
        const std::filesystem::path folder{ source.parent_path().parent_path().parent_path().parent_path().parent_path() / "build" / "bench" };
        std::error_code error{};
        std::filesystem::create_directories(folder, error);

        const std::filesystem::path file{ folder / fileName };
        std::ofstream out{ file, std::ios::trunc };
        if (!out.is_open()) return {};

        for (const std::string& line : lines) out << line << "\n";
        return file;
    }
}