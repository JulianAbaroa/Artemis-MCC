export module Views.Oracle.System:Report;

import Views.Oracle.Type;
import std;

namespace
{
    using Views::Oracle::Type::Stats::OracleStats;
}

export namespace Views::Oracle::System
{
    // return: One line with the counters of both comparisons, the cost of the engine and the lag.
    auto FormatSummary(const OracleStats& stats) -> std::string;

    // return: The text of the report file: the header, the counters and the worst differences.
    // param header: Lines written first (when it started, the map, the settings).
    auto FormatReport(const OracleStats& stats, std::string_view header) -> std::string;
}