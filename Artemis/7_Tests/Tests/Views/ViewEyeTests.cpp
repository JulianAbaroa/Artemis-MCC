#include <gtest/gtest.h>

import Tests.Support.Bench;
import Tests.Support.FilmPass;
import Tests.Support.Pipeline;
import Tests.Support.ViewProbe;
import std;

// The eye of the local player derived from its biped tag (A) and from the bone of its head (B), against the camera the engine gives it
// (F4.B, D309 proposal). The table goes to stdout and to build/bench/view_eye.txt. It only fails when a standing player is more than
// k_MaxStandingError from either eye at the 99th percentile: that means the measure is wrong.

namespace
{
    using Tests::Support::Bench::Stats;
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::ViewProbe::EyeProbe;
    using Tests::Support::ViewProbe::EyeState;

    // Films with a local player: the nine goldens and the two bench films.
    constexpr std::array<std::string_view, 11> k_Films{ "armas_a", "armas_b", "cgb_displace_1min", "ctf_c", "ctf_d", "explosivos", "lifts",
        "powerups", "suelo", "BenchFilms/alien8v8", "BenchFilms/forge650" };

    // Only one record in this many goes through the pipeline (eight decisions of D273), to keep the test short: the eye does not depend on history.
    constexpr std::size_t k_DecisionEvery{ 32 };

    // World units at the 99th percentile that a standing player may be away from either eye.
    constexpr double k_MaxStandingError{ 0.25 };

    struct FilmEye
    {
        std::string Skip{};
        std::string Error{};
        std::size_t Records{};
        EyeProbe Probe{};
    };

    auto MeasureFilm(std::string_view film) -> FilmEye
    {
        FilmEye result{};
        const FilmOutcome outcome{ RunFilmPass(film, false, [&](const Chain& chain, std::size_t) { result.Probe.Observe(chain); }, {}, k_DecisionEvery) };
        result.Skip = outcome.Skip;
        result.Error = outcome.Error;
        result.Records = outcome.Records;
        return result;
    }
}

TEST(DISABLED_ViewEyeTests, DerivedEyeMatchesLocalCamera)
{
    std::vector<std::future<FilmEye>> jobs{};
    for (const std::string_view film : k_Films) jobs.push_back(std::async(std::launch::async, MeasureFilm, film));

    std::vector<std::string> report{};
    report.push_back(std::format("Eye of the local player against its camera, a decision every {} records, {}.", k_DecisionEvery, Tests::Support::Bench::Configuration()));
    std::size_t measured{ 0 };
    std::string skipped{};
    for (std::size_t index{ 0 }; index < k_Films.size(); ++index)
    {
        const FilmEye result{ jobs[index].get() };
        report.push_back("");
        report.push_back(std::format("Film {}: {} records.", k_Films[index], result.Records));
        EXPECT_TRUE(result.Error.empty()) << k_Films[index] << ": " << result.Error;
        if (!result.Skip.empty())
        {
            report.push_back("  skipped: " + result.Skip);
            skipped = std::string{ k_Films[index] } + ": " + result.Skip;
            continue;
        }

        for (const std::string& line : result.Probe.Lines()) report.push_back(line);

        const Stats standingA{ result.Probe.StatsA(EyeState::StandingStill) };
        const Stats standingB{ result.Probe.StatsB(EyeState::StandingStill) };
        if (standingA.N > 0) ++measured;
        EXPECT_LE(standingA.P99, k_MaxStandingError) << k_Films[index] << ": eye A, standing still";
        EXPECT_LE(standingB.P99, k_MaxStandingError) << k_Films[index] << ": eye B, standing still";
    }

    const std::filesystem::path file{ Tests::Support::Bench::WriteReport("view_eye.txt", report) };
    for (const std::string& line : report) std::println("{}", line);
    EXPECT_FALSE(file.empty());
    if (measured == 0 && !skipped.empty()) GTEST_SKIP() << skipped;
    EXPECT_GT(measured, 0u) << "no film had a local player standing still";
}