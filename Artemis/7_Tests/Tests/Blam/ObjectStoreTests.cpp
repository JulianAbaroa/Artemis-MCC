#include <gtest/gtest.h>

import Common.Math.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import std;

// The stores of Blam.Runtime.Object that reuse memory between ticks (S2.O, D276): the pool of published tables of ObjectTableStore and
// the kept buffers of the bone and damage stores. What readers see must be the same as with a fresh allocation every tick.

namespace
{
    using Blam::Runtime::Object::State::BoneMatricesStore;
    using Blam::Runtime::Object::State::DamageSectionsStore;
    using Blam::Runtime::Object::State::ObjectTableStore;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrix;
    using Blam::Runtime::Object::Type::DamageSection::DamageSectionState;

    constexpr std::uint32_t k_Handle{ 0x10001 };

    // Makes the object of k_Handle with the given X in its origin, so a table is told from another by its value.
    auto Store(ObjectTableStore& store, float x) -> void
    {
        if (!store.HasObject(k_Handle))
        {
            AliveObject object{};
            object.Handle = k_Handle;
            store.AddObject(k_Handle, object);
        }

        store.UpdateObjects([&](std::uint32_t, AliveObject& object) { object.Origin.X = x; });
    }

    auto OriginOf(const std::shared_ptr<const ObjectTable>& table) -> float
    {
        return table->at(k_Handle).Origin.X;
    }
}

TEST(ObjectTableStoreTests, TableHeldByAReaderKeepsItsValuesAcrossPublishes)
{
    ObjectTableStore store{};
    Store(store, 1.0f);
    store.Publish();
    const auto held{ store.Acquire() };

    for (int tick{ 2 }; tick <= 4; ++tick)
    {
        Store(store, static_cast<float>(tick));
        store.Publish();
    }

    EXPECT_FLOAT_EQ(OriginOf(held), 1.0f);
    EXPECT_FLOAT_EQ(OriginOf(store.Acquire()), 4.0f);
}

TEST(ObjectTableStoreTests, EveryTableHeldAtOnceKeepsItsValuesPastThePoolCapacity)
{
    ObjectTableStore store{};
    std::vector<std::shared_ptr<const ObjectTable>> held{};
    for (int tick{ 1 }; tick <= 10; ++tick)
    {
        Store(store, static_cast<float>(tick));
        store.Publish();
        held.push_back(store.Acquire());
    }

    for (std::size_t index{ 0 }; index < held.size(); ++index) EXPECT_FLOAT_EQ(OriginOf(held[index]), static_cast<float>(index + 1));
}

TEST(ObjectTableStoreTests, ReleasedTablesAreReusedInsteadOfAllocated)
{
    ObjectTableStore store{};
    std::set<const ObjectTable*> addresses{};
    for (int tick{ 1 }; tick <= 20; ++tick)
    {
        Store(store, static_cast<float>(tick));
        store.Publish();
        // The reader of the tick lets go before the next one.
        const auto table{ store.Acquire() };
        addresses.insert(table.get());
        EXPECT_FLOAT_EQ(OriginOf(table), static_cast<float>(tick));
    }

    // The published table is never written over, so two tables alternate.
    EXPECT_EQ(addresses.size(), 2u);
}

TEST(ObjectTableStoreTests, CleanupLeavesNothingToAcquire)
{
    ObjectTableStore store{};
    Store(store, 1.0f);
    store.Publish();
    ASSERT_NE(store.Acquire(), nullptr);

    store.Cleanup();

    EXPECT_EQ(store.Acquire(), nullptr);
    Store(store, 2.0f);
    store.Publish();
    EXPECT_FLOAT_EQ(OriginOf(store.Acquire()), 2.0f);
}

TEST(BoneMatricesStoreTests, HandleNotWrittenSinceTheLastClearIsNull)
{
    BoneMatricesStore store{};
    store.Write(k_Handle).Matrices.resize(3);
    ASSERT_NE(store.Get(k_Handle), nullptr);
    EXPECT_EQ(store.Get(k_Handle)->Matrices.size(), 3u);

    store.Clear();

    EXPECT_EQ(store.Get(k_Handle), nullptr);
}

TEST(BoneMatricesStoreTests, WriteAfterClearHasOnlyTheNewValues)
{
    BoneMatricesStore store{};
    auto& first{ store.Write(k_Handle) };
    first.Matrices.resize(5);
    for (BoneMatrix& bone : first.Matrices) bone.Scale = 9.0f;
    store.Clear();

    auto& second{ store.Write(k_Handle) };
    EXPECT_TRUE(second.Matrices.empty());
    second.Matrices.resize(2);
    EXPECT_EQ(second.Matrices.size(), 2u);
    EXPECT_FLOAT_EQ(second.Matrices[0].Scale, 0.0f);
    EXPECT_EQ(store.Get(k_Handle), &second);
}

TEST(BoneMatricesStoreTests, DiscardedTableIsNull)
{
    BoneMatricesStore store{};
    store.Write(k_Handle).Matrices.resize(1);
    store.Discard(k_Handle);
    EXPECT_EQ(store.Get(k_Handle), nullptr);

    store.Discard(0x10002);
    store.Write(k_Handle);
    EXPECT_NE(store.Get(k_Handle), nullptr);
}

TEST(BoneMatricesStoreTests, HandlesNotWrittenForLongAreDroppedWhenTheStoreOutgrowsTheLiveOnes)
{
    BoneMatricesStore store{};
    for (std::uint32_t handle{ 1 }; handle <= 500; ++handle) store.Write(handle).Matrices.resize(1);
    store.Clear();
    store.Write(1).Matrices.resize(1);
    store.Clear();
    store.Write(1).Matrices.resize(1);

    EXPECT_NE(store.Get(1), nullptr);
    EXPECT_EQ(store.Get(2), nullptr);
    store.Write(2).Matrices.resize(4);
    EXPECT_EQ(store.Get(2)->Matrices.size(), 4u);
}

TEST(DamageSectionsStoreTests, HandleNotWrittenSinceTheLastClearIsNull)
{
    DamageSectionsStore store{};
    store.Write(k_Handle).Sections.resize(2);
    ASSERT_NE(store.Get(k_Handle), nullptr);

    store.Clear();

    EXPECT_EQ(store.Get(k_Handle), nullptr);
}

TEST(DamageSectionsStoreTests, WriteAfterClearHasNoLeftoversWithFewerSections)
{
    DamageSectionsStore store{};
    auto& first{ store.Write(k_Handle) };
    first.Sections.resize(6);
    first.Sections[5].Vitality = 7.0f;
    first.Regions.Size = 16;
    first.Regions.Offset = 0x40;
    first.Regions.Bytes.assign(16, 0xAB);
    store.Clear();

    auto& second{ store.Write(k_Handle) };
    EXPECT_TRUE(second.Sections.empty());
    EXPECT_TRUE(second.Regions.Bytes.empty());
    EXPECT_EQ(second.Regions.Size, 0u);
    EXPECT_EQ(second.Regions.Offset, 0u);

    second.Sections.resize(2);
    EXPECT_EQ(store.Get(k_Handle)->Sections.size(), 2u);
    EXPECT_FLOAT_EQ(store.Get(k_Handle)->Sections[1].Vitality, 0.0f);
}

TEST(DamageSectionsStoreTests, DiscardedTableIsNull)
{
    DamageSectionsStore store{};
    store.Write(k_Handle).Sections.resize(1);
    store.Discard(k_Handle);

    EXPECT_EQ(store.Get(k_Handle), nullptr);
}