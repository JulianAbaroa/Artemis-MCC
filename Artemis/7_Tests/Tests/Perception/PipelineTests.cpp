#include <gtest/gtest.h>

import Common.Log.Type;
import Contract.Bridge.Type;
import Bridge.Codec.Pipeline.System;
import Bridge.Codec.Tick.Type;
import Blam.Layout.Snapshot.Type;
import Tests.Support.Pipeline;
import std;

// The pure chain of M5b (D198, D200): Tables, Environment (with Relations), Egocentric and Export built and run without the DLL, from
// Snapshots made by hand and a tag index written by hand. No .map, no MCC, no Platform and no Service.

namespace
{
    using Common::Log::Type::LogLevel;
    using Bridge::Codec::Tick::Type::Tick;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Contract::Bridge::Type::BridgeTick;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::Pipeline::FillSyntheticTags;
    using Tests::Support::Pipeline::WithBipedAndPlayer;

    using Bridge::Codec::Pipeline::System::k_PipelineStageNames;
    using Tests::Support::Pipeline::k_BipedHandle;
    using Tests::Support::Pipeline::k_PlayerHandle;

    // Services that write "Cleanup completed." when their layer is cleaned up, with the layer that owns them.
    const std::array<std::string_view, 23> k_CleanupTags{
        "[ClassifierService]", "[ObjectGraphService]",
        "[VitalityDetailsService]", "[ZoneDetailsService]", "[TeleporterDetailsService]", "[LiftDetailsService]",
        "[BarrierDetailsService]", "[EntitiesService]", "[PlayersService]", "[ObjectiveDetailsService]", "[CollidableService]", "[AimService]",
        "[SelfService]", "[AffordanceService]", "[RaycastService]",
        "[ObjectTableService]", "[PlayerTableService]", "[InteractionService]",
        "[MapLoaderService]", "[MapBuilderService]",
        "[WorldBuilder]", "[DefinitionsBuilder]", "[VitalityBuilder]",
    };

    // The seven layers of the pure chain over one Log and a tag index written by hand (D197).
    class PipelineTests : public ::testing::Test, protected Chain
    {
    protected:
        auto SetUp() -> void override
        {
            FillSyntheticTags(m_Map);
        }

        // Runs the pipeline as Mod::Thread::System does (D205) and returns the Tick it assembled.
        auto RunTick(const Snapshot& snapshot, std::uint64_t generation) -> std::shared_ptr<const Tick>
        {
            m_Export.m_PipelineService.Run(snapshot, generation, {});
            return m_Export.m_TickStore.Acquire();
        }

        auto Flatten(const Tick& tick) const -> std::unique_ptr<BridgeTick>
        {
            auto flat{ std::make_unique<BridgeTick>() };
            m_Export.m_FlatService.Flatten(tick, *flat);
            return flat;
        }

        // Cleans up the six layers in the order Mod registers them (D200).
        auto CleanupAll() -> void
        {
            m_Environment.Cleanup();
            m_Egocentric.Cleanup();
            m_Export.Cleanup();
            m_Tables.Cleanup();
            m_Map.Cleanup();
            m_Resolved.Cleanup();
        }

        auto CountOfCleanup(std::string_view tag) const -> std::size_t
        {
            return static_cast<std::size_t>(std::ranges::count_if(m_Sink.m_Messages, [tag](const auto& message) {
                return message.Level == LogLevel::Info && message.Tag == tag && message.Message == "Cleanup completed.";
            }));
        }
    };
}

TEST_F(PipelineTests, PipelineBuildsWithoutTheDll)
{
    // Nothing ran yet: no layer published anything and nothing was logged.
    EXPECT_EQ(m_Export.m_TickStore.Acquire(), nullptr);
    EXPECT_EQ(m_Tables.m_ObjectStore.Acquire(), nullptr);
    EXPECT_EQ(m_Environment.m_ClassifierStore.Acquire(), nullptr);
    EXPECT_EQ(m_Environment.m_PlayersStore.Acquire(), nullptr);
    EXPECT_EQ(m_Egocentric.m_SelfStore.Acquire(), nullptr);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Error), 0u);
}

TEST_F(PipelineTests, EmptySnapshotGivesAnEmptyBridgeTick)
{
    const auto tick{ this->RunTick(Snapshot{}, 1) };
    ASSERT_NE(tick, nullptr);
    EXPECT_EQ(tick->Generation, 1u);

    const auto flat{ this->Flatten(*tick) };
    EXPECT_EQ(flat->Header.PlayerCount, 0u);
    EXPECT_EQ(flat->Header.EntityCount, 0u);
    EXPECT_EQ(flat->Header.EventCount, 0u);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Warning), 0u);
    EXPECT_EQ(m_Sink.CountOf(LogLevel::Error), 0u);
}

TEST_F(PipelineTests, BipedAndPlayerReachTheBridgeTick)
{
    const auto tick{ this->RunTick(WithBipedAndPlayer(), 7) };
    ASSERT_NE(tick, nullptr);

    const auto flat{ this->Flatten(*tick) };
    EXPECT_EQ(flat->Header.Generation, 7u);

    ASSERT_EQ(flat->Header.PlayerCount, 1u);
    EXPECT_EQ(flat->Players[0].Handle, k_PlayerHandle);
    EXPECT_EQ(flat->Players[0].BipedHandle, k_BipedHandle);

    ASSERT_GE(flat->Header.EntityCount, 1u);
    const auto entities{ std::span<const Contract::Bridge::Type::BridgeEntity>{ flat->Entities }.first(flat->Header.EntityCount) };
    EXPECT_TRUE(std::ranges::any_of(entities, [](const auto& entity) { return entity.Handle == k_BipedHandle; }));
}

TEST_F(PipelineTests, CleanupForgetsTheTick)
{
    const auto tick{ this->RunTick(WithBipedAndPlayer(), 1) };
    ASSERT_NE(tick, nullptr);
    ASSERT_EQ(this->Flatten(*tick)->Header.PlayerCount, 1u);

    this->CleanupAll();

    EXPECT_EQ(m_Export.m_TickStore.Acquire(), nullptr);

    const auto empty{ this->RunTick(Snapshot{}, 2) };
    ASSERT_NE(empty, nullptr);
    const auto flat{ this->Flatten(*empty) };
    EXPECT_EQ(flat->Header.PlayerCount, 0u);
    EXPECT_EQ(flat->Header.EntityCount, 0u);
    EXPECT_EQ(flat->Header.EventCount, 0u);

    // One line per service that writes it today (22), none from Export.
    for (const std::string_view tag : k_CleanupTags)
    {
        EXPECT_EQ(this->CountOfCleanup(tag), 1u) << tag;
    }
    const auto total{ std::ranges::count_if(m_Sink.m_Messages, [](const auto& message) { return message.Message == "Cleanup completed."; }) };
    EXPECT_EQ(static_cast<std::size_t>(total), k_CleanupTags.size());
}

// Environment owns the classification and the object graph since M8 C.1 (D241): its Cleanup() empties them with the rest.
class EnvironmentLayerTests : public ::testing::Test, protected Chain
{
protected:
    auto SetUp() -> void override
    {
        FillSyntheticTags(m_Map);
    }
};

TEST_F(EnvironmentLayerTests, CleanupEmptiesTheClassificationAndTheGraph)
{
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {});
    ASSERT_NE(m_Environment.m_ClassifierStore.Acquire(), nullptr);
    ASSERT_NE(m_Environment.m_ObjectGraphStore.Acquire(), nullptr);
    ASSERT_NE(m_Environment.m_EntitiesStore.Acquire(), nullptr);

    m_Environment.Cleanup();

    EXPECT_EQ(m_Environment.m_ClassifierStore.Acquire(), nullptr);
    EXPECT_EQ(m_Environment.m_ObjectGraphStore.Acquire(), nullptr);
    EXPECT_EQ(m_Environment.m_EntitiesStore.Acquire(), nullptr);
}

TEST_F(PipelineTests, EndStageOncePerStage)
{
    std::size_t calls{ 0 };
    m_Export.m_PipelineService.Run(Snapshot{}, 1, [&calls] { ++calls; });

    EXPECT_EQ(k_PipelineStageNames.size(), 21u);
    EXPECT_EQ(calls, k_PipelineStageNames.size());
}