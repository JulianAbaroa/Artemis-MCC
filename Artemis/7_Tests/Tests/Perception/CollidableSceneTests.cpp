#include <gtest/gtest.h>

import Bridge.Codec.Pipeline.System;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Object.Type;
import Perception.Collidable.Type;
import Tests.Support.Pipeline;
import std;

// The scene of collidables is built apart from the pipeline (D224): the pipeline only fills the tables, BuildScene reads them.

namespace
{
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrix;
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
    EXPECT_EQ(k_PipelineStageNames.size(), 23u);
    EXPECT_FALSE(std::ranges::contains(k_PipelineStageNames, std::string_view{ "Collidable" }));

    std::size_t calls{ 0 };
    m_Export.m_PipelineService.Run(Snapshot{}, 1, [&calls] { ++calls; });
    EXPECT_EQ(calls, k_PipelineStageNames.size());
}

TEST_F(CollidableSceneTests, UpdatePartsKeyFillsTheKeyOfARowAndTellsWhetherItChanged)
{
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {});
    const auto objects{ m_Tables.m_ObjectStore.Acquire() };
    ASSERT_NE(objects, nullptr);
    AliveObject row{ objects->at(k_BipedHandle) };
    auto& service{ m_Environment.m_CollidableService };

    // A new key is filled from the row, and the same row again changes nothing.
    CollidablePartsKey first{};
    EXPECT_TRUE(service.UpdatePartsKey(row, first, false));
    EXPECT_FALSE(service.UpdatePartsKey(row, first, true));
    EXPECT_EQ(first.Handle, k_BipedHandle);
    EXPECT_EQ(first.Position.X, row.BoundingCenter.X);
    EXPECT_EQ(first.Forward.Y, row.Forward.Y);
    EXPECT_EQ(first.Up.Z, row.Up.Z);
    EXPECT_EQ(first.DatumIndex, row.DatumIndex);

    // The key updated in place is the key made from nothing.
    CollidablePartsKey fresh{};
    service.UpdatePartsKey(row, fresh, false);
    EXPECT_EQ(first, fresh);

    // The row fields of the pose are part of the key.
    row.BoundingCenter.X += 1.0f;
    EXPECT_TRUE(service.UpdatePartsKey(row, first, true));
    EXPECT_EQ(first.Position.X, row.BoundingCenter.X);
    EXPECT_FALSE(service.UpdatePartsKey(row, first, true));
    row.Forward.Y += 0.5f;
    EXPECT_TRUE(service.UpdatePartsKey(row, first, true));
    row.Up.Z -= 0.5f;
    EXPECT_TRUE(service.UpdatePartsKey(row, first, true));
    EXPECT_FALSE(service.UpdatePartsKey(row, first, true));

    // The same tag resolves to the same model and states, so a key of the same datum index lends them and another one does not.
    ASSERT_NE(row.DatumIndex, 0u);
    static const int marker{};
    CollidablePartsKey lent{ first };
    lent.CollModel = reinterpret_cast<decltype(lent.CollModel)>(&marker);
    EXPECT_FALSE(service.UpdatePartsKey(row, lent, true));
    EXPECT_EQ(lent.CollModel, reinterpret_cast<decltype(lent.CollModel)>(&marker));

    lent.DatumIndex += 1;
    EXPECT_TRUE(service.UpdatePartsKey(row, lent, true));
    EXPECT_EQ(lent.DatumIndex, row.DatumIndex);
    EXPECT_EQ(lent.CollModel, first.CollModel);
}

TEST_F(CollidableSceneTests, UpdatePartsKeyTakesBonesOnlyWhenOneIsValidAndTheDamageOnlyWithATable)
{
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {});
    const auto objects{ m_Tables.m_ObjectStore.Acquire() };
    ASSERT_NE(objects, nullptr);
    const AliveObject row{ objects->at(k_BipedHandle) };
    auto& service{ m_Environment.m_CollidableService };

    m_Tables.m_BoneMatricesStore.Discard(k_BipedHandle);
    m_Tables.m_DamageSectionsStore.Discard(k_BipedHandle);
    CollidablePartsKey key{};
    service.UpdatePartsKey(row, key, false);
    EXPECT_FALSE(key.HasBones);
    EXPECT_TRUE(key.Bones.empty());
    EXPECT_FALSE(key.HasDamage);
    EXPECT_TRUE(key.DamageRegions.empty());

    // Matrices that are not valid (the invisible objects) are no bones: the key is the one of a rigid object.
    BoneMatrix valid{};
    valid.Scale = 1.0f;
    valid.Rotation = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    BoneMatrix invalid{};
    m_Tables.m_BoneMatricesStore.Write(k_BipedHandle).Matrices = { invalid, invalid };
    EXPECT_FALSE(service.UpdatePartsKey(row, key, true));
    EXPECT_FALSE(key.HasBones);
    EXPECT_TRUE(key.Bones.empty());

    // One valid matrix: all the matrices go to the key, and a change of any byte is a change of key.
    m_Tables.m_BoneMatricesStore.Write(k_BipedHandle).Matrices = { invalid, valid };
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    EXPECT_TRUE(key.HasBones);
    ASSERT_EQ(key.Bones.size(), 2u);
    EXPECT_EQ(key.Bones[1].Scale, 1.0f);
    EXPECT_FALSE(service.UpdatePartsKey(row, key, true));
    valid.Translation[2] = 3.0f;
    m_Tables.m_BoneMatricesStore.Write(k_BipedHandle).Matrices = { invalid, valid };
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    EXPECT_EQ(key.Bones[1].Translation[2], 3.0f);
    m_Tables.m_BoneMatricesStore.Write(k_BipedHandle).Matrices = { valid };
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    EXPECT_EQ(key.Bones.size(), 1u);

    // The damage bytes go to the key when the object has a table.
    m_Tables.m_DamageSectionsStore.Write(k_BipedHandle).Regions.Bytes = { 1, 2, 3 };
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    EXPECT_TRUE(key.HasDamage);
    EXPECT_EQ(key.DamageRegions, (std::vector<std::uint8_t>{ 1, 2, 3 }));
    EXPECT_FALSE(service.UpdatePartsKey(row, key, true));
    m_Tables.m_DamageSectionsStore.Write(k_BipedHandle).Regions.Bytes = { 1, 9, 3 };
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    m_Tables.m_DamageSectionsStore.Discard(k_BipedHandle);
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    EXPECT_FALSE(key.HasDamage);
    EXPECT_TRUE(key.DamageRegions.empty());
}

TEST_F(CollidableSceneTests, UpdatePartsKeyKeepsTheNameOnlyForARowWithoutTag)
{
    m_Export.m_PipelineService.Run(WithBipedAndPlayer(), 1, {});
    const auto objects{ m_Tables.m_ObjectStore.Acquire() };
    ASSERT_NE(objects, nullptr);
    AliveObject row{ objects->at(k_BipedHandle) };
    auto& service{ m_Environment.m_CollidableService };

    CollidablePartsKey key{};
    service.UpdatePartsKey(row, key, false);
    EXPECT_TRUE(key.TagName.empty());

    row.DatumIndex = 0;
    row.TagName = "scenery\\rock";
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    EXPECT_EQ(key.DatumIndex, 0u);
    EXPECT_EQ(key.TagName, "scenery\\rock");
    EXPECT_FALSE(service.UpdatePartsKey(row, key, true));

    row.TagName = "scenery\\tree";
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    EXPECT_EQ(key.TagName, "scenery\\tree");

    // With a datum index the name is not part of the key.
    row.DatumIndex = 0x1234;
    EXPECT_TRUE(service.UpdatePartsKey(row, key, true));
    EXPECT_TRUE(key.TagName.empty());
    row.TagName = "other";
    EXPECT_FALSE(service.UpdatePartsKey(row, key, true));
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