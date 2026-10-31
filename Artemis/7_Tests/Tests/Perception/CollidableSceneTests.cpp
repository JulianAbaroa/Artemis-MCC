#include <gtest/gtest.h>

import Bridge.Codec.Pipeline.System;
import Blam.Layout.Snapshot.Type;
import Tests.Support.Pipeline;
import std;

// The scene of collidables is built apart from the pipeline (D224): the pipeline only fills the tables, BuildScene reads them.

namespace
{
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::Pipeline::FillSyntheticTags;
    using Tests::Support::Pipeline::WithBipedAndPlayer;

    using Bridge::Codec::Pipeline::System::k_PipelineStageNames;

    class CollidableSceneTests : public ::testing::Test, protected Chain
    {
    protected:
        auto SetUp() -> void override
        {
            FillSyntheticTags(m_Map);
        }
    };
}

TEST_F(CollidableSceneTests, BuildSceneBeforeAnyTickIsEmpty)
{
    EXPECT_EQ(m_Environment.m_CollidableService.BuildScene(), std::nullopt);
}

TEST_F(CollidableSceneTests, BuildSceneHasOneCollidablePerObject)
{
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {});

    const auto objects{ m_Tables.m_ObjectStore.Acquire() };
    ASSERT_NE(objects, nullptr);
    ASSERT_FALSE(objects->empty());

    const auto scene{ m_Environment.m_CollidableService.BuildScene() };
    ASSERT_TRUE(scene.has_value());
    ASSERT_EQ(scene->size(), objects->size());

    std::set<std::uint32_t> expected{};
    for (const auto& [handle, object] : *objects) expected.insert(handle);

    std::set<std::uint32_t> actual{};
    for (const auto& collidable : *scene)
    {
        actual.insert(collidable.Handle);

        const auto found{ objects->find(collidable.Handle) };
        ASSERT_NE(found, objects->end());
        EXPECT_EQ(collidable.TagName, found->second.TagName);
        EXPECT_EQ(collidable.Position.X, found->second.BoundingCenter.X);
        EXPECT_EQ(collidable.Position.Y, found->second.BoundingCenter.Y);
        EXPECT_EQ(collidable.Position.Z, found->second.BoundingCenter.Z);
    }
    EXPECT_EQ(actual, expected);
}

TEST_F(CollidableSceneTests, ThePipelineDoesNotBuildTheScene)
{
    EXPECT_EQ(k_PipelineStageNames.size(), 24u);
    EXPECT_FALSE(std::ranges::contains(k_PipelineStageNames, std::string_view{ "Collidable" }));

    std::size_t calls{ 0 };
    m_Export.m_PipelineService.Run(Snapshot{}, 1, [&calls] { ++calls; });
    EXPECT_EQ(calls, 24u);
}