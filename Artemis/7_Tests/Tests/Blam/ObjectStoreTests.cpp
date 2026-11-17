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
        store.Edit([&] {
            AliveObject object{};
            object.Handle = k_Handle;
            store.AddRow(k_Handle, object).Origin.X = x;
            store.MarkChanged(k_Handle);
        });
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

// The teardown does not wait for a publish that is slow (D297): its Cleanup can come while the AI thread has the table armed. What the
// publish was about to put out belongs to the map that is gone, so nothing may stay published after the Cleanup (RV.1, S9).
TEST(ObjectTableStoreTests, CleanupDuringPublishLeavesNothingPublished)
{
    ObjectTableStore store{};
    Store(store, 1.0f);

    std::thread teardown{};
    std::atomic<bool> isCleaned{ false };
    store.SetPublishProbe([&] {
        teardown = std::thread([&] {
            store.Cleanup();
            isCleaned.store(true);
        });

        // The Cleanup is done at once when the store is free, and waits for the end of the Publish when the lock is held.
        for (int wait{ 0 }; wait < 50 && !isCleaned.load(); ++wait) std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
    });
    store.Publish();
    teardown.join();
    store.SetPublishProbe({});

    EXPECT_EQ(store.Acquire(), nullptr);

    // The next map publishes as usual.
    Store(store, 2.0f);
    store.Publish();
    ASSERT_NE(store.Acquire(), nullptr);
    EXPECT_FLOAT_EQ(OriginOf(store.Acquire()), 2.0f);
}

namespace
{
    // What a table must hold: handle -> origin X (the row also carries 2 x X in WorldOrigin.Y and X as the placement index).
    using Reference = std::map<std::uint32_t, float>;

    auto MakeRow(std::uint32_t handle, float x) -> AliveObject
    {
        AliveObject object{};
        object.Handle = handle;
        object.Origin.X = x;
        object.WorldOrigin.Y = 2.0f * x;
        object.Placement = Blam::Runtime::Object::Type::Placement::Placement{ .Index = static_cast<std::uint16_t>(x) };
        return object;
    }

    auto SameAs(const ObjectTable& table, const Reference& reference) -> bool
    {
        if (table.size() != reference.size()) return false;

        for (const auto& [handle, x] : reference)
        {
            const auto it{ table.find(handle) };
            if (it == table.end()) return false;

            const AliveObject& row{ it->second };
            if (row.Handle != handle || row.Origin.X != x || row.WorldOrigin.Y != 2.0f * x || !row.Placement || row.Placement->Index != static_cast<std::uint16_t>(x)) return false;
        }

        return true;
    }
}

// Publish copies only the rows that changed since the pooled table it writes over was published. Whatever the objects do (arrive, leave, change,
// a slot reused under another salt) and whoever holds an old table, the published table equals the whole table and a held table does not move.
TEST(ObjectTableStoreTests, PublishByRowsEqualsTheWholeTableWhileReadersHoldOldOnes)
{
    ObjectTableStore store{};
    std::mt19937 random{ 303 };
    Reference reference{};
    std::uint32_t salt{ 1 };
    std::vector<std::pair<std::shared_ptr<const ObjectTable>, Reference>> held{};

    const auto pick{ [&]() -> std::uint32_t {
        auto it{ reference.begin() };
        std::advance(it, random() % reference.size());
        return it->first;
    } };

    for (int tick{ 1 }; tick <= 400; ++tick)
    {
        store.Edit([&] {
            const std::uint32_t operations{ random() % 6 };
            for (std::uint32_t operation{ 0 }; operation < operations; ++operation)
            {
                const std::uint32_t kind{ random() % 4 };
                if (kind == 0 || reference.empty())
                {
                    // An arrival, often in a slot that has an object: the slot is reused under a new salt.
                    const std::uint32_t slot{ 1 + (random() % 40) };
                    const std::uint32_t handle{ (salt++ << 16) | slot };
                    const float x{ static_cast<float>(tick * 100 + operation) };
                    for (auto it{ reference.begin() }; it != reference.end(); ++it)
                    {
                        if ((it->first & 0xFFFF) == slot)
                        {
                            reference.erase(it);
                            break;
                        }
                    }
                    store.AddRow(handle, MakeRow(handle, x));
                    reference[handle] = x;
                }
                else if (kind == 1)
                {
                    const std::uint32_t handle{ pick() };
                    store.RemoveRow(handle);
                    reference.erase(handle);
                }
                else
                {
                    const std::uint32_t handle{ pick() };
                    const float x{ static_cast<float>(tick * 100 + operation) };
                    AliveObject* row{ store.FindRow(handle) };
                    ASSERT_NE(row, nullptr);
                    *row = MakeRow(handle, x);
                    store.MarkChanged(handle);
                    reference[handle] = x;
                }
            }
        });
        store.Publish();

        const auto published{ store.Acquire() };
        ASSERT_NE(published, nullptr);
        ASSERT_TRUE(SameAs(*published, reference)) << "tick " << tick;

        // Readers: some keep the table for a few ticks, some let go of everything.
        if (random() % 4 == 0) held.emplace_back(published, reference);
        if (held.size() > 5 || random() % 40 == 0) held.erase(held.begin(), held.begin() + (random() % (held.size() + 1)));
        for (const auto& [table, expected] : held) ASSERT_TRUE(SameAs(*table, expected)) << "held table, tick " << tick;
    }
}

TEST(ObjectTableStoreTests, EditRowsFollowTheSlotAndNeedNoSearch)
{
    ObjectTableStore store{};
    store.Edit([&] {
        EXPECT_EQ(store.FindRow(k_Handle), nullptr);
        store.AddRow(k_Handle, MakeRow(k_Handle, 1.0f));
        ASSERT_NE(store.FindRow(k_Handle), nullptr);
        EXPECT_EQ(store.SlotHandle(k_Handle), k_Handle);

        // Another salt of the same slot does not find the row, and adding it takes the old one out.
        const std::uint32_t other{ k_Handle + 0x10000 };
        EXPECT_EQ(store.FindRow(other), nullptr);
        store.AddRow(other, MakeRow(other, 2.0f));
        EXPECT_EQ(store.FindRow(k_Handle), nullptr);
        ASSERT_NE(store.FindRow(other), nullptr);
    });
    store.Publish();

    const auto table{ store.Acquire() };
    EXPECT_EQ(table->size(), 1u);
    EXPECT_TRUE(table->contains(k_Handle + 0x10000));
}

TEST(BoneMatricesStoreTests, TableStaysVisibleUntilItIsDiscarded)
{
    BoneMatricesStore store{};
    store.Write(k_Handle).Matrices.resize(3);

    for (int tick{ 0 }; tick < 3; ++tick)
    {
        store.Prune();
        ASSERT_NE(store.Get(k_Handle), nullptr);
        EXPECT_EQ(store.Get(k_Handle)->Matrices.size(), 3u);
    }

    store.Discard(k_Handle);
    EXPECT_EQ(store.Get(k_Handle), nullptr);
}

TEST(BoneMatricesStoreTests, WriteOverAnExistingTableHasOnlyTheNewValues)
{
    BoneMatricesStore store{};
    auto& first{ store.Write(k_Handle) };
    first.Matrices.resize(5);
    for (BoneMatrix& bone : first.Matrices) bone.Scale = 9.0f;

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

TEST(BoneMatricesStoreTests, HandlesWithNoTableAreDroppedWhenTheyOutnumberTheLiveOnes)
{
    BoneMatricesStore store{};
    for (std::uint32_t handle{ 1 }; handle <= 500; ++handle) store.Write(handle).Matrices.resize(1);
    for (std::uint32_t handle{ 2 }; handle <= 500; ++handle) store.Discard(handle);
    store.Prune();

    EXPECT_NE(store.Get(1), nullptr);
    EXPECT_EQ(store.Get(2), nullptr);
    store.Write(2).Matrices.resize(4);
    EXPECT_EQ(store.Get(2)->Matrices.size(), 4u);
    EXPECT_EQ(store.Get(1)->Matrices.size(), 1u);
}

TEST(DamageSectionsStoreTests, TableStaysVisibleUntilItIsDiscarded)
{
    DamageSectionsStore store{};
    store.Write(k_Handle).Sections.resize(2);

    for (int tick{ 0 }; tick < 3; ++tick)
    {
        store.Prune();
        ASSERT_NE(store.Get(k_Handle), nullptr);
        EXPECT_EQ(store.Get(k_Handle)->Sections.size(), 2u);
    }

    store.Discard(k_Handle);
    EXPECT_EQ(store.Get(k_Handle), nullptr);
}

TEST(DamageSectionsStoreTests, WriteOverAnExistingTableHasNoLeftoversWithFewerSections)
{
    DamageSectionsStore store{};
    auto& first{ store.Write(k_Handle) };
    first.Sections.resize(6);
    first.Sections[5].Vitality = 7.0f;
    first.Regions.Size = 16;
    first.Regions.Offset = 0x40;
    first.Regions.Bytes.assign(16, 0xAB);

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