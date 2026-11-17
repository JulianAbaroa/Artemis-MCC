#include <gtest/gtest.h>

import Common.Engine.Type;
import Perception.Entities.Type;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;
import Views.Raycast.System;
import Views.Raycast.Type;
import Tests.Support.FilmPass;
import Tests.Support.Pipeline;
import std;

// The exclusion of each player built from the object graph is the one BuildSelfExclusion scans the tables for (F4.B). Only the decision
// records go through the pipeline: one every k_DecisionEvery records (four decisions of D273), enough rows for the sets to be compared.

namespace
{
    using Tests::Support::FilmPass::FilmOutcome;
    using Tests::Support::FilmPass::RunFilmPass;
    using Tests::Support::Pipeline::Chain;
    using Views::Raycast::System::BuildGraphExclusions;
    using Views::Raycast::System::BuildSelfExclusion;
    using Views::Raycast::Type::PlayerExclusions;
    using Views::Raycast::Type::SelfExclusion;

    using Common::Engine::Type::k_NoHandle;

    constexpr std::size_t k_DecisionEvery{ 16 };

    // What a film gave: the rows compared and the ones that differ, with the first differences described.
    struct ExclusionCounts
    {
        std::size_t Decisions{};
        std::size_t Rows{};
        std::size_t Differences{};
        std::size_t Handles{};
        std::string Skip{};
        std::string Error{};
        std::vector<std::string> Messages{};
    };

    auto Describe(const SelfExclusion& exclusion) -> std::string
    {
        std::vector<std::uint32_t> handles{ exclusion.begin(), exclusion.end() };
        std::ranges::sort(handles);
        std::string text{};
        for (const std::uint32_t handle : handles) text += std::format("{:08X} ", handle);
        return text;
    }

    auto CompareFilm(std::string_view film) -> ExclusionCounts
    {
        ExclusionCounts counts{};
        const FilmOutcome outcome{ RunFilmPass(film, false, [&](const Chain& chain, std::size_t record)
        {
            const auto players{ chain.m_Environment.m_PlayersStore.Acquire() };
            const auto entities{ chain.m_Environment.m_EntitiesStore.Acquire() };
            const auto nodes{ chain.m_Environment.m_ObjectGraphStore.Acquire() };
            if (!players || !entities || !nodes) return;

            ++counts.Decisions;
            const PlayerExclusions graph{ BuildGraphExclusions(*players, *entities, *nodes) };
            for (std::size_t index{ 0 }; index < players->Rows.size(); ++index)
            {
                const auto& player{ players->Rows[index] };
                if (!player.IsAlive()) continue;

                const SelfExclusion scan{ BuildSelfExclusion(chain.m_Environment.m_PlayersStore, chain.m_Environment.m_EntitiesStore,
                    chain.m_Tables.m_ObjectStore, player.Handle, player.BipedHandle) };
                ++counts.Rows;
                counts.Handles += scan.size();
                if (graph[index] == scan) continue;

                ++counts.Differences;
                if (counts.Messages.size() < 5)
                {
                    counts.Messages.push_back(std::format("record {} player {:08X}: graph [{}] scan [{}]", record, player.Handle,
                        Describe(graph[index]), Describe(scan)));
                }
            }
        }, {}, k_DecisionEvery) };
        counts.Error = outcome.Error;
        counts.Skip = outcome.Skip;
        return counts;
    }

    auto Check(std::string_view film, const ExclusionCounts& counts) -> void
    {
        EXPECT_TRUE(counts.Error.empty()) << film << ": " << counts.Error;
        if (!counts.Skip.empty())
        {
            GTEST_SKIP() << film << ": " << counts.Skip;
        }

        EXPECT_GT(counts.Rows, 0u) << film << ": no live row was compared";
        EXPECT_EQ(counts.Differences, 0u) << film << ": " << counts.Differences << " of " << counts.Rows << " rows differ";
        for (const std::string& message : counts.Messages) ADD_FAILURE() << film << ": " << message;
        std::println("ViewExclusionTests {}: {} decisions, {} live rows, {} handles, {} differences.", film, counts.Decisions, counts.Rows,
            counts.Handles, counts.Differences);
    }
}

TEST(DISABLED_ViewExclusionTests, GraphMatchesScan)
{
    std::future<ExclusionCounts> ctfC{ std::async(std::launch::async, CompareFilm, std::string_view{ "ctf_c" }) };
    std::future<ExclusionCounts> forge{ std::async(std::launch::async, CompareFilm, std::string_view{ "BenchFilms/forge650" }) };
    Check("ctf_c", ctfC.get());
    Check("BenchFilms/forge650", forge.get());
}