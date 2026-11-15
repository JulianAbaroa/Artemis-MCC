#include <gtest/gtest.h>

import Bridge.Codec.Pipeline.System;
import Blam.Layout.Snapshot.Type;
import Perception.Collidable.Type;
import Tests.Support.Pipeline;
import std;

// The scene of collidables is built apart from the pipeline (D224): the pipeline only fills the tables, BuildScene reads them.

namespace
{
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Perception::Collidable::Type::CollidablePartsKey;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::Pipeline::FillSyntheticTags;
    using Tests::Support::Pipeline::WithBipedAndPlayer;
    using Tests::Support::Pipeline::k_BipedHandle;

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

TEST_F(CollidableSceneTests, MakePartsKeyFailsForAnUnknownHandleAndIsStableForALiveOne)
{
    CollidablePartsKey first{};
    CollidablePartsKey second{};
    EXPECT_FALSE(m_Environment.m_CollidableService.MakePartsKey(k_BipedHandle, first, nullptr));

    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {});
    EXPECT_FALSE(m_Environment.m_CollidableService.MakePartsKey(0x12345678, first, nullptr));
    EXPECT_EQ(first, CollidablePartsKey{});

    ASSERT_TRUE(m_Environment.m_CollidableService.MakePartsKey(k_BipedHandle, first, nullptr));
    ASSERT_TRUE(m_Environment.m_CollidableService.MakePartsKey(k_BipedHandle, second, &first));
    EXPECT_EQ(first.Handle, k_BipedHandle);
    EXPECT_EQ(first, second);

    // The key reads what CollectPartsFor reads: the row fields of the object.
    const auto objects{ m_Tables.m_ObjectStore.Acquire() };
    ASSERT_NE(objects, nullptr);
    const auto& row{ objects->at(k_BipedHandle) };
    EXPECT_EQ(first.Position.X, row.BoundingCenter.X);
    EXPECT_EQ(first.Forward.Y, row.Forward.Y);
    EXPECT_EQ(first.Up.Z, row.Up.Z);
    EXPECT_EQ(first.DatumIndex, row.DatumIndex);

    // The same tag resolves to the same model and states, so a key of the same datum index lends them and another one does not.
    ASSERT_NE(row.DatumIndex, 0u);
    static const int marker{};
    CollidablePartsKey lent{};
    lent.DatumIndex = row.DatumIndex;
    lent.CollModel = reinterpret_cast<decltype(lent.CollModel)>(&marker);
    CollidablePartsKey third{};
    ASSERT_TRUE(m_Environment.m_CollidableService.MakePartsKey(k_BipedHandle, third, &lent));
    EXPECT_EQ(third.CollModel, lent.CollModel);

    lent.DatumIndex += 1;
    ASSERT_TRUE(m_Environment.m_CollidableService.MakePartsKey(k_BipedHandle, third, &lent));
    EXPECT_EQ(third.CollModel, first.CollModel);
}

TEST(CollidablePartsKey, EqualityIsByBytesOverEveryField)
{
    CollidablePartsKey key{};
    key.Handle = 7;
    key.DatumIndex = 0x1234;
    key.Bones.resize(2);
    key.HasBones = true;
    key.DamageRegions = { 1, 2, 3 };
    key.HasDamage = true;
    EXPECT_EQ(key, key);

    const std::vector<std::pair<std::string_view, std::function<void(CollidablePartsKey&)>>> changes{
        { "handle", [](CollidablePartsKey& k) { k.Handle = 8; } },
        { "datum index", [](CollidablePartsKey& k) { k.DatumIndex = 0x1235; } },
        { "name", [](CollidablePartsKey& k) { k.TagName = "x"; } },
        { "position", [](CollidablePartsKey& k) { k.Position.Z = 1.0f; } },
        { "forward", [](CollidablePartsKey& k) { k.Forward.X = 1.0f; } },
        { "up", [](CollidablePartsKey& k) { k.Up.Y = 1.0f; } },
        { "has bones", [](CollidablePartsKey& k) { k.HasBones = false; } },
        { "a bone", [](CollidablePartsKey& k) { k.Bones[1].Rotation[4] = 0.5f; } },
        { "bone count", [](CollidablePartsKey& k) { k.Bones.pop_back(); } },
        { "has damage", [](CollidablePartsKey& k) { k.HasDamage = false; } },
        { "damage byte", [](CollidablePartsKey& k) { k.DamageRegions[2] = 9; } },
        { "damage size", [](CollidablePartsKey& k) { k.DamageRegions.push_back(0); } },
        { "collision model", [](CollidablePartsKey& k) { static const int model{}; k.CollModel = reinterpret_cast<decltype(k.CollModel)>(&model); } },
        { "region states", [](CollidablePartsKey& k) { static const int states{}; k.States = reinterpret_cast<decltype(k.States)>(&states); } },
    };
    for (const auto& [name, change] : changes)
    {
        CollidablePartsKey other{ key };
        change(other);
        EXPECT_NE(key, other) << name;
    }

    // Bytes, not values: a sign of zero is a change and a NaN equals itself.
    CollidablePartsKey negative{ key };
    negative.Position.X = -0.0f;
    EXPECT_NE(key, negative);

    CollidablePartsKey nan{ key };
    nan.Bones[0].Scale = std::numeric_limits<float>::quiet_NaN();
    const CollidablePartsKey copy{ nan };
    EXPECT_EQ(nan, copy);
}