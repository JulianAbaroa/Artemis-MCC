export module Views.Oracle.System:Record;

import Views.Oracle.Type;
import std;

namespace
{
    using Views::Oracle::Type::Sample::OracleComparison;
    using Views::Oracle::Type::Sample::OracleSample;
    using Views::Oracle::Type::Stats::CostHistogram;
    using Views::Oracle::Type::Stats::OracleStats;
    using Views::Oracle::Type::Stats::OracleSummary;
}

export namespace Views::Oracle::System
{
    // Counts a sample and its comparison, keeps it in the worst list if it is among the worst, and adds the cost of the engine.
    auto Record(OracleStats& stats, const OracleSample& sample, const OracleComparison& comparison) -> void;

    // Counts the lag of a batch, in ticks of the game thread.
    auto RecordLag(OracleStats& stats, std::uint64_t lagTicks) -> void;

    // return: The cost under which the fraction of the calls fell, in microseconds. 0 if there are none.
    // param fraction: 0 to 1.
    auto Percentile(const CostHistogram& histogram, double fraction) -> double;

    auto Summarize(const OracleStats& stats) -> OracleSummary;
}