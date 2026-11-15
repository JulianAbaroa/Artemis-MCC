#include <gtest/gtest.h>

import Common.Geometry.System;
import Common.Math.Type;
import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Collidable.Type;
import Views.Self.Type;
import Views.Raycast.Type;
import Views.Raycast.State;
import Views.Raycast.System;
import Tests.Support.Pipeline;
import std;

// The shared two level ray scene (D298, RS.0) with made-up meshes: poses, exclusion, ties, the fixed tree and the exactness against
// a linear reference written here. RS.1 (D299): the reuse of the parts, the store between ticks and the Self update. RS.1b (D300): the
// objects at rest, the filter of the fixed tree and its rebuilds.

namespace
{
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Physics;
    using Blam::MapModel::Definitions::Type::Coll::Coll;
    using Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Placement::Placement;
    using Perception::Collidable::Type::CollidablePart;
    using Perception::Collidable::Type::CollidablePartsKey;
    using Tests::Support::Pipeline::Chain;
    using Views::Raycast::State::RaycastSceneStore;
    using Views::Raycast::System::ArmFixedScene;
    using Views::Raycast::System::ArmMobileScene;
    using Views::Raycast::System::CastScene;
    using Views::Raycast::System::GatherSceneObjects;
    using Views::Raycast::System::PoseRay;
    using Views::Raycast::System::PrepareParts;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::FixedRebuildRule;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneGatherCounters;
    using Views::Raycast::Type::SceneKeyFunction;
    using Views::Raycast::Type::SceneObject;
    using Views::Raycast::Type::SceneObjects;
    using Views::Raycast::Type::SceneParts;
    using Views::Raycast::Type::ScenePartsCache;
    using Views::Raycast::Type::ScenePartsFunction;
    using Views::Raycast::Type::SceneRef;
    using Views::Raycast::Type::SceneRefs;
    using Views::Raycast::Type::SelfExclusion;

    using Views::Raycast::Type::Constant::k_FixedRebuildGathers;
    using Views::Raycast::Type::Constant::k_FixedRebuildMaxGathers;
    using Views::Raycast::Type::Constant::k_FixedRebuildPending;
    using Views::Raycast::Type::Constant::k_RestGathers;

    constexpr float k_Range{ 50.0f };

    // The rule of D300: a build is due as soon as one object waits and the spacing passed. The tests of the trees use it to keep their
    // sequences short. The rule of the constants has its own tests.
    constexpr FixedRebuildRule k_EveryWaiting{ k_FixedRebuildGathers, 1, k_FixedRebuildMaxGathers };

    // A square of 2 x 2 m on the plane x = 1 of its local space.
    auto MakeSquare() -> CollMesh
    {
        CollMesh mesh{};
        mesh.Triangles.push_back(Triangle{ { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } });
        mesh.Triangles.push_back(Triangle{ { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f } });
        mesh.LocalMin = { 1.0f, -1.0f, -1.0f };
        mesh.LocalMax = { 1.0f, 1.0f, 1.0f };
        mesh.RegionIndex = 3;
        return mesh;
    }

    // A quarter turn around Z, then a translation.
    auto QuarterTurn(float x, float y, float z) -> std::array<float, 12>
    {
        return { 0.0f, -1.0f, 0.0f, x, 1.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 1.0f, z };
    }

    // The parts the objects of the tests view, as the cache of a gather owns them. They live as long as the test binary.
    auto PartsPool() -> std::deque<std::vector<CollidablePart>>&
    {
        static std::deque<std::vector<CollidablePart>> pool{};
        return pool;
    }

    auto MakeObject(std::uint32_t handle, const CollMesh& mesh, const std::array<float, 12>& transform, bool isFixed) -> SceneObject
    {
        CollidablePart part{};
        part.Source = &mesh;
        part.Transform = transform;
        PartsPool().push_back({ part });
        return SceneObject{ handle, isFixed, PartsPool().back() };
    }

    // Moves the first part of an object that views a vector of the pool.
    auto MovePart(SceneObject& object, const std::array<float, 12>& transform) -> void
    {
        const_cast<CollidablePart&>(object.Parts[0]).Transform = transform;
    }

    // The objects sorted by handle, as the gather gives them.
    auto Objects(std::initializer_list<SceneObject> list) -> SceneObjects
    {
        SceneObjects objects{};
        for (const SceneObject& object : list) objects.push_back(object);
        std::ranges::sort(objects, [](const SceneObject& a, const SceneObject& b) { return a.Handle < b.Handle; });
        return objects;
    }

    // One decision over the objects: the fixed tree and the mobile tree, on a copy because arming marks the objects.
    auto Arm(RaycastScene& scene, const SceneObjects& objects) -> void
    {
        SceneObjects marked{ objects };
        ArmFixedScene(scene, marked, k_EveryWaiting);
        ArmMobileScene(scene, marked);
    }

    // The reference of the random rays: every triangle of every part, in the local space of the part, with no hierarchy and no culling.
    // note: The part order and the local pose are the ones the scene uses. At equal distance the lowest handle, part and triangle wins.
    auto CastLinear(const SceneObjects& objects, const Vec3& origin, const Vec3& direction, float maxDistance) -> RaycastHit
    {
        RaycastHit best{};
        std::array<std::uint32_t, 3> bestKey{};
        for (const SceneObject& object : objects)
        {
            const std::vector<CachedPart> parts{ PrepareParts(object.Parts) };
            for (std::size_t partIndex{ 0 }; partIndex < parts.size(); ++partIndex)
            {
                Vec3 localOrigin{};
                Vec3 localDirection{};
                PoseRay(parts[partIndex].ToLocal, origin, direction, localOrigin, localDirection);

                const std::vector<Triangle>& triangles{ parts[partIndex].Source->Triangles };
                for (std::size_t triangleIndex{ 0 }; triangleIndex < triangles.size(); ++triangleIndex)
                {
                    float distance{ 0.0f };
                    const float limit{ best.Hit ? best.Distance : maxDistance };
                    if (!Common::Geometry::System::RayIntersectsTriangle(localOrigin, localDirection, triangles[triangleIndex], limit, distance)) continue;

                    const std::array<std::uint32_t, 3> key{ object.Handle, static_cast<std::uint32_t>(partIndex), static_cast<std::uint32_t>(triangleIndex) };
                    if (best.Hit && !(distance < best.Distance || (distance == best.Distance && key < bestKey))) continue;

                    best.Hit = true;
                    best.Kind = HitKind::Dynamic;
                    best.Distance = distance;
                    best.ObjectHandle = object.Handle;
                    best.RegionIndex = parts[partIndex].Source->RegionIndex;
                    best.Point = { origin.X + direction.X * distance, origin.Y + direction.Y * distance, origin.Z + direction.Z * distance };
                    bestKey = key;
                }
            }
        }

        return best;
    }

    auto Cast(const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin, const Vec3& direction) -> RaycastHit
    {
        RaycastHit best{};
        CastScene(scene, exclusion, origin, direction, k_Range, best, nullptr);
        return best;
    }
}

TEST(RaycastScene, HitsAMeshOfAnObjectThatIsRotatedAndMoved)
{
    const CollMesh mesh{ MakeSquare() };
    const SceneObjects objects = Objects({ MakeObject(0xA1, mesh, QuarterTurn(10.0f, 0.0f, 0.0f), false) });
    RaycastScene scene{};
    Arm(scene, objects);

    // The square stands on the plane y = 1 around x = 10.
    const RaycastHit hit{ Cast(scene, SelfExclusion{}, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }) };
    ASSERT_TRUE(hit.Hit);
    EXPECT_EQ(hit.Kind, HitKind::Dynamic);
    EXPECT_EQ(hit.ObjectHandle, 0xA1u);
    EXPECT_EQ(hit.RegionIndex, 3);
    EXPECT_NEAR(hit.Distance, 6.0f, 1.0e-4f);

    const RaycastHit miss{ Cast(scene, SelfExclusion{}, { 13.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }) };
    EXPECT_FALSE(miss.Hit);
}

TEST(RaycastScene, ExclusionHidesTheObjectOfTheAgent)
{
    const CollMesh mesh{ MakeSquare() };
    const SceneObjects objects = Objects({ MakeObject(0xA1, mesh, QuarterTurn(10.0f, 0.0f, 0.0f), false), MakeObject(0xA2, mesh, QuarterTurn(10.0f, 4.0f, 0.0f), true) });
    RaycastScene scene{};
    Arm(scene, objects);

    EXPECT_EQ(Cast(scene, SelfExclusion{}, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).ObjectHandle, 0xA1u);
    EXPECT_EQ(Cast(scene, SelfExclusion{ 0xA1 }, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).ObjectHandle, 0xA2u);
    EXPECT_FALSE(Cast(scene, SelfExclusion{ 0xA1, 0xA2 }, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).Hit);
}

TEST(RaycastScene, TieGoesToTheLowestHandleAndAStaticHitStays)
{
    const CollMesh mesh{ MakeSquare() };
    const SceneObjects objects = Objects({ MakeObject(7, mesh, QuarterTurn(10.0f, 0.0f, 0.0f), false), MakeObject(3, mesh, QuarterTurn(10.0f, 0.0f, 0.0f), true) });
    RaycastScene scene{};
    Arm(scene, objects);

    const Vec3 origin{ 10.0f, -5.0f, 0.0f };
    const Vec3 direction{ 0.0f, 1.0f, 0.0f };
    EXPECT_EQ(Cast(scene, SelfExclusion{}, origin, direction).ObjectHandle, 3u);

    RaycastHit best{};
    best.Hit = true;
    best.Kind = HitKind::Static;
    best.Distance = 6.0f;
    CastScene(scene, SelfExclusion{}, origin, direction, k_Range, best, nullptr);
    EXPECT_EQ(best.Kind, HitKind::Static);
    EXPECT_EQ(best.ObjectHandle, 0u);

    // A nearer static hit also stays, and a farther one is replaced.
    best.Distance = 5.0f;
    CastScene(scene, SelfExclusion{}, origin, direction, k_Range, best, nullptr);
    EXPECT_EQ(best.Kind, HitKind::Static);
    best.Distance = 9.0f;
    CastScene(scene, SelfExclusion{}, origin, direction, k_Range, best, nullptr);
    EXPECT_EQ(best.Kind, HitKind::Dynamic);
    EXPECT_NEAR(best.Distance, 6.0f, 1.0e-4f);
}

TEST(RaycastScene, ChangedFixedObjectIsFilteredAtOnceAndTheTreeIsBuiltAgainAfterTheGathers)
{
    const CollMesh mesh{ MakeSquare() };
    SceneObjects objects = Objects({ MakeObject(1, mesh, QuarterTurn(0.0f, 0.0f, 0.0f), true), MakeObject(2, mesh, QuarterTurn(5.0f, 0.0f, 0.0f), false) });
    RaycastScene scene{};

    EXPECT_TRUE(ArmFixedScene(scene, objects, k_EveryWaiting));
    EXPECT_FALSE(ArmFixedScene(scene, objects, k_EveryWaiting));
    EXPECT_EQ(scene.FixedBuilds, 1u);
    EXPECT_EQ(scene.Fixed.Pieces.size(), 1u);
    EXPECT_TRUE(objects[0].IsCovered);
    EXPECT_FALSE(objects[1].IsCovered);

    // A mobile object that moves or leaves does not touch the fixed tree.
    MovePart(objects[1], QuarterTurn(6.0f, 1.0f, 0.0f));
    EXPECT_FALSE(ArmFixedScene(scene, objects, k_EveryWaiting));
    EXPECT_TRUE(scene.FixedFilter.empty());

    // Object 1 stands at y = 1 and its old leaf would hit at 6 m. It moves 3 m: the old leaf stops hitting in this very gather, the new
    // parts are in the mobile tree, and the tree is not built again.
    const Vec3 origin{ 0.0f, -5.0f, 0.0f };
    const Vec3 up{ 0.0f, 1.0f, 0.0f };
    ASSERT_NEAR(Cast(scene, SelfExclusion{}, origin, up).Distance, 6.0f, 1.0e-4f);
    MovePart(objects[0], QuarterTurn(0.0f, 3.0f, 0.0f));
    EXPECT_FALSE(ArmFixedScene(scene, objects, k_EveryWaiting));
    ArmMobileScene(scene, objects);
    EXPECT_EQ(scene.FixedFilter, std::vector<std::uint32_t>{ 1u });
    EXPECT_EQ(scene.FixedFiltered, 1u);
    EXPECT_FALSE(objects[0].IsCovered);
    EXPECT_EQ(scene.Mobile.Pieces.size(), 2u);
    const RaycastHit moved{ Cast(scene, SelfExclusion{}, origin, up) };
    EXPECT_EQ(moved.ObjectHandle, 1u);
    EXPECT_NEAR(moved.Distance, 9.0f, 1.0e-4f);

    // No build until k_FixedRebuildGathers gathers passed since the last one.
    while (scene.GathersSinceBuild + 1 < k_FixedRebuildGathers) EXPECT_FALSE(ArmFixedScene(scene, objects, k_EveryWaiting));
    EXPECT_EQ(scene.FixedBuilds, 1u);
    EXPECT_FALSE(scene.FixedFilter.empty());

    EXPECT_TRUE(ArmFixedScene(scene, objects, k_EveryWaiting));
    ArmMobileScene(scene, objects);
    EXPECT_EQ(scene.FixedBuilds, 2u);
    EXPECT_TRUE(scene.FixedFilter.empty());
    EXPECT_EQ(scene.GathersSinceBuild, 0u);
    EXPECT_EQ(scene.Fixed.Pieces.size(), 1u);
    EXPECT_EQ(scene.Mobile.Pieces.size(), 1u);
    EXPECT_TRUE(objects[0].IsCovered);
    EXPECT_NEAR(Cast(scene, SelfExclusion{}, origin, up).Distance, 9.0f, 1.0e-4f);
    EXPECT_EQ(scene.Meshes.size(), 1u);
}

TEST(RaycastScene, AnObjectThatDisappearsFromTheFixedTreeStopsHittingInThatGather)
{
    const CollMesh mesh{ MakeSquare() };
    SceneObjects objects = Objects({ MakeObject(1, mesh, QuarterTurn(0.0f, 0.0f, 0.0f), true), MakeObject(2, mesh, QuarterTurn(0.0f, 4.0f, 0.0f), true) });
    RaycastScene scene{};
    Arm(scene, objects);

    const Vec3 origin{ 0.0f, -5.0f, 0.0f };
    const Vec3 up{ 0.0f, 1.0f, 0.0f };
    EXPECT_EQ(Cast(scene, SelfExclusion{}, origin, up).ObjectHandle, 1u);

    // Object 1 is gone: no rebuild, and the ray goes on to object 2.
    objects.erase(objects.begin());
    SceneObjects marked{ objects };
    EXPECT_FALSE(ArmFixedScene(scene, marked, k_EveryWaiting));
    ArmMobileScene(scene, marked);
    EXPECT_EQ(scene.FixedFilter, std::vector<std::uint32_t>{ 1u });
    EXPECT_EQ(Cast(scene, SelfExclusion{}, origin, up).ObjectHandle, 2u);

    // Object 1 leaves rest with the same parts: its leaf in the fixed tree is filtered and the same parts hit from the mobile tree.
    RaycastScene other{};
    SceneObjects both = Objects({ MakeObject(1, mesh, QuarterTurn(0.0f, 0.0f, 0.0f), true), MakeObject(2, mesh, QuarterTurn(0.0f, 4.0f, 0.0f), true) });
    Arm(other, both);
    both[0].IsFixed = false;
    EXPECT_FALSE(ArmFixedScene(other, both, k_EveryWaiting));
    ArmMobileScene(other, both);
    EXPECT_EQ(other.FixedFilter, std::vector<std::uint32_t>{ 1u });
    EXPECT_EQ(other.Mobile.Pieces.size(), 1u);
    const RaycastHit hit{ Cast(other, SelfExclusion{}, origin, up) };
    EXPECT_EQ(hit.ObjectHandle, 1u);
    EXPECT_NEAR(hit.Distance, 6.0f, 1.0e-4f);
}

TEST(RaycastScene, ANewObjectAtRestWaitsInTheMobileTreeUntilTheTreeIsBuilt)
{
    const CollMesh mesh{ MakeSquare() };
    SceneObjects objects = Objects({ MakeObject(1, mesh, QuarterTurn(0.0f, 0.0f, 0.0f), true) });
    RaycastScene scene{};
    EXPECT_TRUE(ArmFixedScene(scene, objects, k_EveryWaiting));

    // A second object at rest is not in the tree: it hits from the mobile tree and nothing is built.
    objects = Objects({ MakeObject(1, mesh, QuarterTurn(0.0f, 0.0f, 0.0f), true), MakeObject(2, mesh, QuarterTurn(0.0f, 4.0f, 0.0f), true) });
    EXPECT_FALSE(ArmFixedScene(scene, objects, k_EveryWaiting));
    ArmMobileScene(scene, objects);
    EXPECT_EQ(scene.FixedPending, 1u);
    EXPECT_TRUE(objects[0].IsCovered);
    EXPECT_FALSE(objects[1].IsCovered);
    EXPECT_EQ(scene.Fixed.Pieces.size(), 1u);
    EXPECT_EQ(scene.Mobile.Pieces.size(), 1u);
    EXPECT_EQ(Cast(scene, SelfExclusion{}, { 0.0f, 2.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).ObjectHandle, 2u);

    while (scene.GathersSinceBuild + 1 < k_FixedRebuildGathers) EXPECT_FALSE(ArmFixedScene(scene, objects, k_EveryWaiting));
    EXPECT_TRUE(ArmFixedScene(scene, objects, k_EveryWaiting));
    ArmMobileScene(scene, objects);
    EXPECT_EQ(scene.Fixed.Pieces.size(), 2u);
    EXPECT_TRUE(scene.Mobile.Pieces.empty());
    EXPECT_TRUE(objects[1].IsCovered);
}

TEST(RaycastScene, StoreCleanupMarksAndTheNextCallForgetsMeshesTreesAndParts)
{
    const CollMesh mesh{ MakeSquare() };
    SceneObjects objects = Objects({ MakeObject(1, mesh, QuarterTurn(10.0f, 0.0f, 0.0f), true), MakeObject(2, mesh, QuarterTurn(20.0f, 0.0f, 0.0f), false) });
    RaycastSceneStore store{};
    Arm(store.Scene(), objects);
    store.Parts().emplace(1u, SceneParts{ CollidablePartsKey{}, {}, false, 5 });

    // Object 1 moves: it goes to the filter, and the scene keeps its counters.
    MovePart(objects[0], QuarterTurn(10.0f, 1.0f, 0.0f));
    ArmFixedScene(store.Scene(), objects, k_EveryWaiting);
    ArmMobileScene(store.Scene(), objects);
    ASSERT_EQ(store.Scene().FixedFilter.size(), 1u);
    ASSERT_GT(store.Scene().GathersSinceBuild, 0u);
    ASSERT_TRUE(Cast(store.Scene(), SelfExclusion{}, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).Hit);
    ASSERT_FALSE(store.Scene().Meshes.empty());

    // Cleanup only marks: what the update thread holds is untouched until it asks again.
    RaycastScene& held{ store.Scene() };
    store.Cleanup();
    EXPECT_FALSE(held.Meshes.empty());

    RaycastScene& scene{ store.Scene() };
    EXPECT_TRUE(scene.Meshes.empty());
    EXPECT_TRUE(scene.Fixed.Pieces.empty());
    EXPECT_TRUE(scene.Mobile.Pieces.empty());
    EXPECT_EQ(scene.FixedBuilds, 0u);
    EXPECT_TRUE(scene.FixedFilter.empty());
    EXPECT_TRUE(scene.FixedPoses.empty());
    EXPECT_EQ(scene.GathersSinceBuild, 0u);
    EXPECT_EQ(scene.FixedPending, 0u);
    EXPECT_EQ(scene.FixedFiltered, 0u);
    EXPECT_TRUE(store.Parts().empty());
    EXPECT_FALSE(Cast(scene, SelfExclusion{}, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).Hit);

    // It arms again from nothing, and a second call does not forget again.
    EXPECT_TRUE(ArmFixedScene(scene, objects, k_EveryWaiting));
    EXPECT_FALSE(store.Scene().Meshes.empty());
}

namespace
{
    // Objects of the tick for the gather: their key and their parts come from tables the test changes between gathers.
    struct GatherFixture
    {
        CollMesh Mesh{ MakeSquare() };
        SceneRefs Refs{};
        std::unordered_map<std::uint32_t, CollidablePartsKey> Keys{};
        std::unordered_map<std::uint32_t, std::array<float, 12>> Poses{};
        std::unordered_map<std::uint32_t, int> PartsCalls{};

        // Parts of an object, each 3 m further along y than the one before. One if not set.
        std::unordered_map<std::uint32_t, std::size_t> PartCounts{};
        ScenePartsCache Reuse{};

        auto Add(std::uint32_t handle, bool isFixed, float x) -> void
        {
            Refs.push_back({ handle, isFixed });
            CollidablePartsKey key{};
            key.Handle = handle;
            key.DatumIndex = 0x100 + handle;
            key.Position = { x, 0.0f, 0.0f };
            Keys[handle] = key;
            Poses[handle] = QuarterTurn(x, 0.0f, 0.0f);
        }

        // Gathers with the key and parts of the tables, counting the calls to the parts function.
        auto Gather(std::size_t& reused) -> SceneObjects
        {
            SceneGatherCounters counters{};
            const SceneObjects objects{ this->Gather(counters) };
            reused = counters.Reused;
            return objects;
        }

        auto Gather() -> SceneObjects
        {
            std::size_t reused{};
            return this->Gather(reused);
        }

        auto Gather(SceneGatherCounters& counters) -> SceneObjects
        {
            const SceneKeyFunction keyOf{ [&](const SceneRef& ref, CollidablePartsKey& key, bool)
            {
                const auto found{ Keys.find(ref.Handle) };
                if (found == Keys.end()) return false;

                const bool isChanged{ !(key == found->second) };
                key = found->second;
                return isChanged;
            } };
            const ScenePartsFunction partsOf{ [&](std::uint32_t handle) -> std::optional<std::vector<CollidablePart>>
            {
                ++PartsCalls[handle];
                std::vector<CollidablePart> parts{};
                const std::size_t count{ PartCounts.contains(handle) ? PartCounts[handle] : 1 };
                for (std::size_t index{ 0 }; index < count; ++index)
                {
                    CollidablePart part{};
                    part.Source = &Mesh;
                    part.Transform = Poses[handle];
                    part.Transform[7] += 3.0f * static_cast<float>(index);
                    parts.push_back(part);
                }

                return parts;
            } };

            return GatherSceneObjects(Reuse, Refs, keyOf, partsOf, &counters);
        }
    };
}

TEST(RaycastScene, ASameKeyReusesThePartsOfAnyObjectWithoutCollectingThem)
{
    GatherFixture fixture{};
    fixture.Add(5, true, 5.0f);
    fixture.Add(2, true, 2.0f);
    fixture.Add(9, false, 9.0f);

    std::size_t reused{ 99 };
    const SceneObjects first{ fixture.Gather(reused) };
    EXPECT_EQ(reused, 0u);
    ASSERT_EQ(first.size(), 3u);
    EXPECT_EQ(first[0].Handle, 2u);
    EXPECT_EQ(first[2].Handle, 9u);

    const SceneObjects second{ fixture.Gather(reused) };
    EXPECT_EQ(reused, 3u);
    EXPECT_EQ(fixture.PartsCalls[5], 1);
    EXPECT_EQ(fixture.PartsCalls[2], 1);

    // A normal object is reused too, and the result is the one of collecting everything.
    EXPECT_EQ(fixture.PartsCalls[9], 1);
    ASSERT_EQ(second.size(), first.size());
    for (std::size_t index{ 0 }; index < first.size(); ++index)
    {
        EXPECT_EQ(second[index].Handle, first[index].Handle);
        EXPECT_EQ(second[index].IsFixed, first[index].IsFixed);
        ASSERT_EQ(second[index].Parts.size(), first[index].Parts.size());
        EXPECT_EQ(second[index].Parts[0].Transform, first[index].Parts[0].Transform);
        EXPECT_EQ(second[index].Parts[0].Source, first[index].Parts[0].Source);
    }
}

TEST(RaycastScene, ChangingAnyInputOfTheKeyInvalidatesTheReuse)
{
    const std::vector<std::pair<std::string_view, std::function<void(CollidablePartsKey&)>>> changes{
        { "position", [](CollidablePartsKey& key) { key.Position.X += 0.001f; } },
        { "forward", [](CollidablePartsKey& key) { key.Forward.Y = 1.0f; } },
        { "up", [](CollidablePartsKey& key) { key.Up.Z = 1.0f; } },
        { "a bone", [](CollidablePartsKey& key) { key.HasBones = true; key.Bones.back().Translation[2] = 3.0f; } },
        { "the damage", [](CollidablePartsKey& key) { key.HasDamage = true; key.DamageRegions[1] = 7; } },
        { "the tag", [](CollidablePartsKey& key) { key.DatumIndex += 1; } },
    };

    for (const auto& [name, change] : changes)
    {
        GatherFixture fixture{};
        fixture.Add(4, true, 4.0f);
        fixture.Keys[4].HasBones = true;
        fixture.Keys[4].Bones.resize(2);
        fixture.Keys[4].HasDamage = true;
        fixture.Keys[4].DamageRegions = { 1, 2, 3 };

        std::size_t reused{};
        fixture.Gather(reused);
        fixture.Gather(reused);
        EXPECT_EQ(reused, 1u) << name;

        change(fixture.Keys[4]);
        fixture.Poses[4] = QuarterTurn(40.0f, 0.0f, 0.0f);
        const SceneObjects changed{ fixture.Gather(reused) };
        EXPECT_EQ(reused, 0u) << name;
        EXPECT_EQ(fixture.PartsCalls[4], 2) << name;
        ASSERT_EQ(changed.size(), 1u) << name;
        EXPECT_EQ(changed[0].Parts[0].Transform, QuarterTurn(40.0f, 0.0f, 0.0f)) << name;

        // The new key is the one kept now.
        fixture.Gather(reused);
        EXPECT_EQ(reused, 1u) << name;
    }
}

TEST(RaycastScene, AnObjectThatIsGoneLeavesTheReuse)
{
    GatherFixture fixture{};
    fixture.Add(1, true, 1.0f);
    fixture.Add(2, true, 2.0f);
    fixture.Add(3, true, 3.0f);

    std::size_t reused{};
    fixture.Gather(reused);
    ASSERT_EQ(fixture.Reuse.size(), 3u);

    // Handle 1 disappears.
    fixture.Refs.erase(fixture.Refs.begin());
    fixture.Gather(reused);
    EXPECT_EQ(reused, 2u);
    EXPECT_EQ(fixture.Reuse.size(), 2u);
    EXPECT_FALSE(fixture.Reuse.contains(1u));

    // Handle 1 comes back (a new object with the same handle) and is collected again.
    fixture.Refs.push_back({ 1, true });
    fixture.Gather(reused);
    EXPECT_EQ(reused, 2u);
    EXPECT_EQ(fixture.PartsCalls[1], 2);
}

TEST(RaycastScene, AnObjectWithoutPartsIsNotRecollectedEither)
{
    GatherFixture fixture{};
    fixture.Add(1, true, 1.0f);
    const SceneKeyFunction keyOf{ [&](const SceneRef&, CollidablePartsKey& key, bool) { const bool isChanged{ !(key == fixture.Keys[1]) }; key = fixture.Keys[1]; return isChanged; } };
    int calls{ 0 };
    const ScenePartsFunction none{ [&](std::uint32_t) -> std::optional<std::vector<CollidablePart>> { ++calls; return std::vector<CollidablePart>{}; } };

    SceneGatherCounters counters{};
    EXPECT_TRUE(GatherSceneObjects(fixture.Reuse, fixture.Refs, keyOf, none, &counters).empty());
    EXPECT_TRUE(GatherSceneObjects(fixture.Reuse, fixture.Refs, keyOf, none, &counters).empty());
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(counters.Reused, 1u);
    EXPECT_EQ(counters.Collected, 0u);
}

TEST(RaycastScene, ANormalObjectRestsAfterTheRestGathersAndIsNotCollectedAgain)
{
    GatherFixture fixture{};
    fixture.Add(1, false, 1.0f);
    RaycastScene scene{};

    // StillGathers is 0 in the first gather and 1 more for each equal key: it rests when it reaches k_RestGathers, not before.
    for (std::size_t gather{ 0 }; gather <= k_RestGathers; ++gather)
    {
        SceneObjects objects{ fixture.Gather() };
        ASSERT_EQ(objects.size(), 1u) << gather;
        EXPECT_EQ(objects[0].IsFixed, gather == k_RestGathers) << gather;
        EXPECT_EQ(fixture.Reuse.at(1).StillGathers, gather) << gather;

        ArmFixedScene(scene, objects, k_EveryWaiting);
        EXPECT_EQ(scene.Fixed.Pieces.size(), gather == k_RestGathers ? 1u : 0u) << gather;
    }

    EXPECT_EQ(fixture.PartsCalls[1], 1);

    // It stays at rest while the key stays.
    EXPECT_TRUE(fixture.Gather()[0].IsFixed);
    EXPECT_EQ(fixture.PartsCalls[1], 1);

    // A change of key takes it out of rest in that same gather, with its new parts.
    fixture.Keys[1].Position.X = 7.0f;
    fixture.Poses[1] = QuarterTurn(7.0f, 0.0f, 0.0f);
    const SceneObjects changed{ fixture.Gather() };
    EXPECT_FALSE(changed[0].IsFixed);
    EXPECT_EQ(changed[0].Parts[0].Transform, QuarterTurn(7.0f, 0.0f, 0.0f));
    EXPECT_EQ(fixture.Reuse.at(1).StillGathers, 0u);
    EXPECT_EQ(fixture.PartsCalls[1], 2);
}

TEST(RaycastScene, AFixedOrPhasedObjectRestsAtOnceAndLeavesRestWhenItsKeyChanges)
{
    GatherFixture fixture{};
    fixture.Add(1, true, 1.0f);
    fixture.Add(2, false, 2.0f);

    std::size_t reused{};
    SceneObjects objects{ fixture.Gather(reused) };
    ASSERT_EQ(objects.size(), 2u);
    EXPECT_TRUE(objects[0].IsFixed);
    EXPECT_FALSE(objects[1].IsFixed);

    fixture.Keys[1].Position.X = 9.0f;
    objects = fixture.Gather(reused);
    EXPECT_FALSE(objects[0].IsFixed);

    // It rests again in the next gather, as its key is the same as in the last one.
    objects = fixture.Gather(reused);
    EXPECT_TRUE(objects[0].IsFixed);
}

namespace
{
    // Objects that rest, move, enter and leave over 120 gathers: the scene gives the hit of the linear reference for 60 random rays each.
    auto CheckLinearReference(const FixedRebuildRule& rule, std::size_t minBuilds) -> void
    {
        std::mt19937 random{ 300 };
        std::uniform_real_distribution<float> unit{ -1.0f, 1.0f };
        const auto point{ [&](float scale) { return Vec3{ unit(random) * scale, unit(random) * scale, unit(random) * scale }; } };
        const auto transformOf{ [&](const Vec3& where) -> std::array<float, 12>
        {
            const float angle{ unit(random) * 3.1f };
            const float cz{ std::cos(angle) }, sz{ std::sin(angle) };
            return { cz, -sz, 0.0f, where.X, sz, cz, 0.0f, where.Y, 0.0f, 0.0f, 1.0f, where.Z };
        } };

        GatherFixture fixture{};
        fixture.Mesh = CollMesh{};
        fixture.Mesh.RegionIndex = 1;
        fixture.Mesh.LocalMin = { 1.0e9f, 1.0e9f, 1.0e9f };
        fixture.Mesh.LocalMax = { -1.0e9f, -1.0e9f, -1.0e9f };
        for (int index{ 0 }; index < 60; ++index)
        {
            const Vec3 center{ point(2.0f) };
            const Vec3 a{ center.X + unit(random), center.Y + unit(random), center.Z + unit(random) };
            const Vec3 b{ center.X + unit(random), center.Y + unit(random), center.Z + unit(random) };
            const Vec3 c{ center.X + unit(random), center.Y + unit(random), center.Z + unit(random) };
            fixture.Mesh.Triangles.push_back(Triangle{ a, b, c });
            for (const Vec3& p : { a, b, c })
            {
                fixture.Mesh.LocalMin = { (std::min)(fixture.Mesh.LocalMin.X, p.X), (std::min)(fixture.Mesh.LocalMin.Y, p.Y), (std::min)(fixture.Mesh.LocalMin.Z, p.Z) };
                fixture.Mesh.LocalMax = { (std::max)(fixture.Mesh.LocalMax.X, p.X), (std::max)(fixture.Mesh.LocalMax.Y, p.Y), (std::max)(fixture.Mesh.LocalMax.Z, p.Z) };
            }
        }

        constexpr std::uint32_t k_Objects{ 14 };
        for (std::uint32_t handle{ 1 }; handle <= k_Objects; ++handle)
        {
            const Vec3 where{ point(8.0f) };
            fixture.Add(handle, handle % 3 == 0, where.X);
            fixture.Keys[handle].Position = where;
            fixture.Poses[handle] = transformOf(where);
        }

        RaycastScene scene{};
        std::size_t hits{ 0 };
        bool wasFiltered{ false };
        bool wasPending{ false };
        for (int gather{ 0 }; gather < 120; ++gather)
        {
            // A few objects move (their key and parts change), one leaves for a while and comes back.
            for (int moves{ 0 }; moves < 2; ++moves)
            {
                if (unit(random) < 0.2f) continue;

                const std::uint32_t handle{ 1 + static_cast<std::uint32_t>((unit(random) * 0.5f + 0.5f) * (k_Objects - 1)) };
                const Vec3 where{ point(8.0f) };
                fixture.Keys[handle].Position = where;
                fixture.Poses[handle] = transformOf(where);
            }

            if (gather % 17 == 5) std::erase_if(fixture.Refs, [](const auto& ref) { return ref.Handle == 2; });
            if (gather % 17 == 9) fixture.Refs.push_back({ 2, false });

            SceneObjects objects{ fixture.Gather() };
            ArmFixedScene(scene, objects, rule);
            ArmMobileScene(scene, objects);
            wasFiltered = wasFiltered || scene.FixedFiltered > 0;
            wasPending = wasPending || scene.FixedPending > 0;

            for (int index{ 0 }; index < 60; ++index)
            {
                const Vec3 origin{ point(14.0f) };
                Vec3 direction{ unit(random), unit(random), unit(random) };
                const float length{ std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z) };
                direction = { direction.X / length, direction.Y / length, direction.Z / length };

                const RaycastHit expected{ CastLinear(objects, origin, direction, 30.0f) };
                RaycastHit actual{};
                CastScene(scene, SelfExclusion{}, origin, direction, 30.0f, actual, nullptr);

                ASSERT_EQ(actual.Hit, expected.Hit) << "gather " << gather << " ray " << index;
                if (!expected.Hit) continue;

                ++hits;
                EXPECT_EQ(actual.Distance, expected.Distance) << "gather " << gather << " ray " << index;
                EXPECT_EQ(actual.ObjectHandle, expected.ObjectHandle) << "gather " << gather << " ray " << index;
                EXPECT_EQ(actual.RegionIndex, expected.RegionIndex) << "gather " << gather << " ray " << index;
                EXPECT_EQ(actual.Point.X, expected.Point.X) << "gather " << gather << " ray " << index;
            }
        }

        // The test went through every state: leaves filtered, objects waiting, and the fixed tree built more than once.
        EXPECT_GT(hits, 300u);
        EXPECT_TRUE(wasFiltered);
        EXPECT_TRUE(wasPending);
        EXPECT_GE(scene.FixedBuilds, minBuilds);
    }
}

TEST(RaycastScene, GivesTheSameHitAsTheLinearReferenceWhileObjectsRestMoveEnterAndLeave)
{
    CheckLinearReference(k_EveryWaiting, 3);
}

TEST(RaycastScene, GivesTheSameHitAsTheLinearReferenceWhenTheTreeWaitsForManyOrForLong)
{
    // The tree is built after 10 gathers with 3 waiting or after 40 gathers with one: the hits do not depend on when.
    CheckLinearReference(FixedRebuildRule{ 10, 3, 40 }, 2);
}

namespace
{
    // The update of Self over a made-up map with one collision model and one object.
    class RaycastSelfUpdateTests : public ::testing::Test, protected Chain
    {
    protected:
        static constexpr std::uint32_t k_BigHandle{ 0xB1600001 };

        // A wall of 40 x 40 m on the plane x = -20 of the collision model, whose center is at x = 65.
        // note: The aim range without a weapon is 50 m, so the center is out of reach and the wall is at 45 m.
        auto SetUp() -> void override
        {
            Coll coll{};
            coll.TagName = "big\\wall";
            CollMesh mesh{};
            mesh.PermutationIndex = 0;
            mesh.Triangles.push_back(Triangle{ { -20.0f, -20.0f, -20.0f }, { -20.0f, 20.0f, -20.0f }, { -20.0f, 20.0f, 20.0f } });
            mesh.Triangles.push_back(Triangle{ { -20.0f, -20.0f, -20.0f }, { -20.0f, 20.0f, 20.0f }, { -20.0f, -20.0f, 20.0f } });
            mesh.LocalMin = { -20.0f, -20.0f, -20.0f };
            mesh.LocalMax = { -20.0f, 20.0f, 20.0f };
            coll.Meshes.push_back(mesh);
            coll.DefaultPermutationIndex = {};
            coll.BoundsMin = { -20.0f, -20.0f, -20.0f };
            coll.BoundsMax = { 20.0f, 20.0f, 20.0f };

            m_Resolved.m_DefinitionsStore.m_Coll.Add("big\\wall", coll);
            m_Resolved.m_DefinitionsStore.Freeze();
            m_Resolved.m_WorldStore.LinkObjectColl("big\\object", "big\\wall");
            m_Resolved.m_WorldStore.Freeze();

            Views::Self::Type::Self self{};
            self.Handle = 1;
            self.Position = { 0.0f, 0.0f, 0.0f };
            self.Forward = { 1.0f, 0.0f, 0.0f };
            self.Right = { 0.0f, -1.0f, 0.0f };
            self.Up = { 0.0f, 0.0f, 1.0f };
            self.IsAlive = true;
            m_Egocentric.m_SelfStore.Publish(self);
        }

        auto PublishWall(float x, Physics physics) -> void
        {
            AliveObject wall{};
            wall.Handle = k_BigHandle;
            wall.DatumIndex = 0x3000;
            wall.TagName = "big\\object";
            wall.BoundingCenter = { x, 0.0f, 0.0f };
            wall.Forward = { 1.0f, 0.0f, 0.0f };
            wall.Up = { 0.0f, 0.0f, 1.0f };
            wall.Placement = Placement{};
            wall.Placement->Physics = physics;
            m_Tables.m_ObjectStore.RemoveObject(k_BigHandle);
            m_Tables.m_ObjectStore.AddObject(k_BigHandle, wall);
            m_Tables.m_ObjectStore.Publish();
        }

        auto Aim() -> RaycastHit
        {
            m_Egocentric.m_RaycastService.Update();
            const auto published{ m_Egocentric.m_RaycastStore.Acquire() };
            return published ? published->AimHit : RaycastHit{};
        }
    };
}

TEST_F(RaycastSelfUpdateTests, ALargeObjectWhoseCenterIsBeyondTheReachIsHit)
{
    this->PublishWall(65.0f, Physics::Normal);

    const RaycastHit hit{ this->Aim() };
    ASSERT_TRUE(hit.Hit);
    EXPECT_EQ(hit.Kind, HitKind::Dynamic);
    EXPECT_EQ(hit.ObjectHandle, k_BigHandle);
    EXPECT_NEAR(hit.Distance, 45.0f, 1.0e-3f);

    // A normal wall hits from the mobile tree: it is not at rest yet.
    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Parts().size(), 1u);
    EXPECT_TRUE(m_Egocentric.m_RaycastSceneStore.Scene().Fixed.Pieces.empty());
    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().Mobile.Pieces.size(), 1u);
}

TEST_F(RaycastSelfUpdateTests, ANormalObjectThatStaysEntersTheFixedTreeAfterTheRestGathers)
{
    this->PublishWall(65.0f, Physics::Normal);

    // The first gather has StillGathers 0 and each equal one adds 1: it rests when it reaches k_RestGathers.
    for (std::size_t tick{ 0 }; tick <= k_RestGathers; ++tick)
    {
        const RaycastHit hit{ this->Aim() };
        ASSERT_TRUE(hit.Hit) << tick;
        EXPECT_NEAR(hit.Distance, 45.0f, 1.0e-3f) << tick;
        EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().Fixed.Pieces.size(), tick == k_RestGathers ? 1u : 0u) << tick;
        EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().Mobile.Pieces.size(), tick == k_RestGathers ? 0u : 1u) << tick;
    }

    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().FixedBuilds, 1u);
}

TEST_F(RaycastSelfUpdateTests, AFixedObjectKeepsItsPartsAndTheFixedTreeBetweenTicks)
{
    this->PublishWall(65.0f, Physics::Fixed);

    for (int tick{ 0 }; tick < 3; ++tick)
    {
        const RaycastHit hit{ this->Aim() };
        ASSERT_TRUE(hit.Hit) << tick;
        EXPECT_EQ(hit.ObjectHandle, k_BigHandle) << tick;
        EXPECT_NEAR(hit.Distance, 45.0f, 1.0e-3f) << tick;
    }

    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Parts().size(), 1u);
    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().FixedBuilds, 1u);

    // The object moves: the key changes, the old leaf is filtered, the new parts hit from the mobile tree and the fixed tree waits.
    this->PublishWall(66.0f, Physics::Fixed);
    EXPECT_NEAR(this->Aim().Distance, 46.0f, 1.0e-3f);
    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().FixedBuilds, 1u);
    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().FixedFilter.size(), 1u);

    // One handle waits, fewer than k_FixedRebuildPending: the tree is built again with the filter empty when the wait reaches
    // k_FixedRebuildMaxGathers gathers. The gather of the move was the first of the wait.
    for (std::size_t tick{ 1 }; tick < k_FixedRebuildMaxGathers; ++tick)
    {
        EXPECT_NEAR(this->Aim().Distance, 46.0f, 1.0e-3f) << tick;
        EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().FixedBuilds, tick + 1 < k_FixedRebuildMaxGathers ? 1u : 2u) << tick;
    }
    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().FixedBuilds, 2u);
    EXPECT_TRUE(m_Egocentric.m_RaycastSceneStore.Scene().FixedFilter.empty());
}

TEST_F(RaycastSelfUpdateTests, CleanupOfTheServiceForgetsTheSceneOnTheNextUpdate)
{
    this->PublishWall(65.0f, Physics::Fixed);
    ASSERT_TRUE(this->Aim().Hit);
    ASSERT_FALSE(m_Egocentric.m_RaycastSceneStore.Scene().Meshes.empty());

    m_Egocentric.m_RaycastService.Cleanup();
    EXPECT_EQ(m_Egocentric.m_RaycastStore.Acquire(), nullptr);
    EXPECT_TRUE(m_Egocentric.m_RaycastSceneStore.Scene().Meshes.empty());
    EXPECT_TRUE(m_Egocentric.m_RaycastSceneStore.Parts().empty());

    // The map is gone: the object table is empty and Self sees nothing, the scene arms from nothing.
    m_Tables.m_ObjectStore.RemoveObject(k_BigHandle);
    m_Tables.m_ObjectStore.Publish();
    EXPECT_FALSE(this->Aim().Hit);
    EXPECT_TRUE(m_Egocentric.m_RaycastSceneStore.Scene().Meshes.empty());
}

namespace
{
    // Objects 1..count at rest, each on its own spot, sorted by handle.
    auto RestingObjects(const CollMesh& mesh, std::uint32_t count) -> SceneObjects
    {
        SceneObjects objects{};
        for (std::uint32_t handle{ 1 }; handle <= count; ++handle)
        {
            objects.push_back(MakeObject(handle, mesh, QuarterTurn(static_cast<float>(handle) * 4.0f, 0.0f, 0.0f), true));
        }

        return objects;
    }

    // Builds the tree over one object and lets the spacing pass with nothing waiting.
    auto BuiltAndSpaced(RaycastScene& scene, const CollMesh& mesh, const FixedRebuildRule& rule) -> void
    {
        SceneObjects first{ RestingObjects(mesh, 1) };
        ASSERT_TRUE(ArmFixedScene(scene, first, rule));
        for (std::size_t gather{ 0 }; gather < rule.Spacing; ++gather) ASSERT_FALSE(ArmFixedScene(scene, first, rule));
        ASSERT_GE(scene.GathersSinceBuild, rule.Spacing);
        ASSERT_EQ(scene.FixedWaitingGathers, 0u);
    }
}

TEST(RaycastScene, FewerObjectsWaitingThanThePendingThresholdDoNotBuildTheTreeEvenAfterTheSpacing)
{
    static_assert(k_FixedRebuildPending > 1 && k_FixedRebuildMaxGathers > k_FixedRebuildGathers + 10);
    const FixedRebuildRule rule{};
    const CollMesh mesh{ MakeSquare() };
    RaycastScene scene{};
    BuiltAndSpaced(scene, mesh, rule);

    // One object short of the threshold waits more than the spacing and the tree stays.
    SceneObjects waiting{ RestingObjects(mesh, static_cast<std::uint32_t>(k_FixedRebuildPending)) };
    for (std::size_t gather{ 1 }; gather <= k_FixedRebuildGathers + 10; ++gather)
    {
        EXPECT_FALSE(ArmFixedScene(scene, waiting, rule)) << gather;
        EXPECT_EQ(scene.FixedPending, k_FixedRebuildPending - 1) << gather;
        EXPECT_EQ(scene.FixedWaitingGathers, gather) << gather;
    }
    EXPECT_EQ(scene.FixedBuilds, 1u);

    // They hit from the mobile tree meanwhile.
    ArmMobileScene(scene, waiting);
    EXPECT_EQ(scene.Fixed.Pieces.size(), 1u);
    EXPECT_EQ(scene.Mobile.Pieces.size(), k_FixedRebuildPending - 1);

    // The threshold object arrives: the build is due at once, with all of them at rest.
    waiting = RestingObjects(mesh, static_cast<std::uint32_t>(k_FixedRebuildPending) + 1);
    EXPECT_TRUE(ArmFixedScene(scene, waiting, rule));
    EXPECT_EQ(scene.FixedBuilds, 2u);
    EXPECT_EQ(scene.FixedWaitingGathers, 0u);
    ArmMobileScene(scene, waiting);
    EXPECT_EQ(scene.Fixed.Pieces.size(), k_FixedRebuildPending + 1);
    EXPECT_TRUE(scene.Mobile.Pieces.empty());
}

TEST(RaycastScene, HandlesFilteredCountAsWaitingForTheThreshold)
{
    const FixedRebuildRule rule{ 4, 3, 1000 };
    const CollMesh mesh{ MakeSquare() };
    RaycastScene scene{};
    SceneObjects objects{ RestingObjects(mesh, 5) };
    ASSERT_TRUE(ArmFixedScene(scene, objects, rule));
    for (int gather{ 0 }; gather < 4; ++gather) ASSERT_FALSE(ArmFixedScene(scene, objects, rule));

    // Two objects move (filtered) and one more rests (pending): 3 waiting, due at once.
    MovePart(objects[0], QuarterTurn(0.0f, 1.0f, 0.0f));
    MovePart(objects[1], QuarterTurn(0.0f, 1.0f, 0.0f));
    SceneObjects more{ objects };
    more.push_back(MakeObject(6, mesh, QuarterTurn(40.0f, 0.0f, 0.0f), true));
    EXPECT_TRUE(ArmFixedScene(scene, more, rule));
    EXPECT_EQ(scene.FixedFiltered, 2u);
    EXPECT_EQ(scene.FixedPending, 1u);
    EXPECT_TRUE(scene.FixedFilter.empty());
}

TEST(RaycastScene, OneObjectWaitingBuildsTheTreeWhenItsWaitReachesTheMaxGathers)
{
    const FixedRebuildRule rule{};
    const CollMesh mesh{ MakeSquare() };
    RaycastScene scene{};
    BuiltAndSpaced(scene, mesh, rule);

    SceneObjects waiting{ RestingObjects(mesh, 2) };
    for (std::size_t gather{ 1 }; gather <= k_FixedRebuildMaxGathers; ++gather)
    {
        EXPECT_EQ(ArmFixedScene(scene, waiting, rule), gather == k_FixedRebuildMaxGathers) << gather;
    }

    EXPECT_EQ(scene.FixedBuilds, 2u);
    EXPECT_EQ(scene.FixedWaitingGathers, 0u);
    EXPECT_EQ(scene.FixedPending, 1u);
    ArmMobileScene(scene, waiting);
    EXPECT_EQ(scene.Fixed.Pieces.size(), 2u);
}

TEST(RaycastScene, TheWaitStartsOverWhenNothingWaitsAnyMore)
{
    const FixedRebuildRule rule{ 2, 5, 10 };
    const CollMesh mesh{ MakeSquare() };
    RaycastScene scene{};
    BuiltAndSpaced(scene, mesh, rule);

    SceneObjects waiting{ RestingObjects(mesh, 2) };
    for (std::size_t gather{ 1 }; gather <= 9; ++gather) ASSERT_FALSE(ArmFixedScene(scene, waiting, rule));
    EXPECT_EQ(scene.FixedWaitingGathers, 9u);

    // The object that waited is gone: nothing waits, and a new one starts counting from one.
    SceneObjects none{ RestingObjects(mesh, 1) };
    EXPECT_FALSE(ArmFixedScene(scene, none, rule));
    EXPECT_EQ(scene.FixedWaitingGathers, 0u);
    EXPECT_FALSE(ArmFixedScene(scene, waiting, rule));
    EXPECT_EQ(scene.FixedWaitingGathers, 1u);
}

TEST(RaycastScene, TheSpacingStillHoldsWhenTheThresholdIsReachedRightAfterABuild)
{
    const FixedRebuildRule rule{ 6, 2, 1000 };
    const CollMesh mesh{ MakeSquare() };
    RaycastScene scene{};
    SceneObjects first{ RestingObjects(mesh, 1) };
    ASSERT_TRUE(ArmFixedScene(scene, first, rule));

    SceneObjects waiting{ RestingObjects(mesh, 4) };
    for (std::size_t gather{ 1 }; gather < rule.Spacing; ++gather) EXPECT_FALSE(ArmFixedScene(scene, waiting, rule)) << gather;
    EXPECT_TRUE(ArmFixedScene(scene, waiting, rule));
    EXPECT_EQ(scene.FixedBuilds, 2u);
}

TEST(RaycastScene, ThePartsOfAnObjectAfterAKeyChangeAreTheNewOnesWhateverTheirNumber)
{
    // The cache replaces the vector of an object when its key changes, with more or fewer parts. The view of the objects must be the new
    // vector and the scene the one of collecting it, over several rounds that grow and shrink it.
    GatherFixture fixture{};
    fixture.Add(1, false, 0.0f);
    fixture.Add(2, true, 4.0f);
    fixture.Add(3, false, 8.0f);
    RaycastScene scene{};

    const std::array<std::size_t, 8> counts{ 1, 40, 2, 300, 1, 17, 64, 3 };
    for (std::size_t round{ 0 }; round < counts.size(); ++round)
    {
        fixture.PartCounts[1] = counts[round];
        fixture.Keys[1].Position.X += 1.0f;
        fixture.Poses[1] = QuarterTurn(-20.0f, 0.5f * static_cast<float>(round), 0.0f);
        fixture.PartCounts[3] = counts[counts.size() - 1 - round];
        fixture.Keys[3].Position.X += 1.0f;

        SceneObjects objects{ fixture.Gather() };
        ASSERT_EQ(objects.size(), 3u) << round;
        ASSERT_EQ(objects[0].Handle, 1u);
        ASSERT_EQ(objects[0].Parts.size(), counts[round]) << round;
        ASSERT_EQ(objects[2].Parts.size(), counts[counts.size() - 1 - round]) << round;
        EXPECT_EQ(objects[0].Parts.data(), fixture.Reuse.at(1).Parts.data()) << round;
        EXPECT_EQ(objects[0].Parts.back().Transform[7], 0.5f * static_cast<float>(round) + 3.0f * static_cast<float>(counts[round] - 1)) << round;

        ArmFixedScene(scene, objects, k_EveryWaiting);
        ArmMobileScene(scene, objects);

        // A ray along y through the x of object 1 hits the first of its parts, which is where the new transform puts it.
        const Vec3 origin{ -20.0f, -5.0f, 0.0f };
        const Vec3 up{ 0.0f, 1.0f, 0.0f };
        RaycastHit actual{};
        CastScene(scene, SelfExclusion{}, origin, up, 2000.0f, actual, nullptr);
        const RaycastHit expected{ CastLinear(objects, origin, up, 2000.0f) };
        ASSERT_EQ(actual.Hit, expected.Hit) << round;
        EXPECT_EQ(actual.Distance, expected.Distance) << round;
        EXPECT_EQ(actual.ObjectHandle, 1u) << round;
        EXPECT_NEAR(actual.Distance, 5.0f + 0.5f * static_cast<float>(round) + 1.0f, 1.0e-3f) << round;

        // The same ray from behind the farthest part hits the last one of the new vector.
        const Vec3 behind{ -20.0f, 3.0f * static_cast<float>(counts[round]) + 20.0f, 0.0f };
        const Vec3 down{ 0.0f, -1.0f, 0.0f };
        RaycastHit last{};
        CastScene(scene, SelfExclusion{}, behind, down, 2000.0f, last, nullptr);
        EXPECT_EQ(last.Hit, CastLinear(objects, behind, down, 2000.0f).Hit) << round;
        EXPECT_EQ(last.Distance, CastLinear(objects, behind, down, 2000.0f).Distance) << round;
    }
}

TEST(RaycastScene, GivesTheSameHitAsTheLinearReferenceOnRandomRays)
{
    std::mt19937 random{ 298 };
    std::uniform_real_distribution<float> unit{ -1.0f, 1.0f };
    const auto point{ [&](float scale) { return Vec3{ unit(random) * scale, unit(random) * scale, unit(random) * scale }; } };

    CollMesh mesh{};
    mesh.RegionIndex = 1;
    mesh.LocalMin = { 1.0e9f, 1.0e9f, 1.0e9f };
    mesh.LocalMax = { -1.0e9f, -1.0e9f, -1.0e9f };
    for (int index{ 0 }; index < 150; ++index)
    {
        const Vec3 center{ point(3.0f) };
        const Vec3 a{ center.X + unit(random), center.Y + unit(random), center.Z + unit(random) };
        const Vec3 b{ center.X + unit(random), center.Y + unit(random), center.Z + unit(random) };
        const Vec3 c{ center.X + unit(random), center.Y + unit(random), center.Z + unit(random) };
        mesh.Triangles.push_back(Triangle{ a, b, c });
        for (const Vec3& p : { a, b, c })
        {
            mesh.LocalMin = { (std::min)(mesh.LocalMin.X, p.X), (std::min)(mesh.LocalMin.Y, p.Y), (std::min)(mesh.LocalMin.Z, p.Z) };
            mesh.LocalMax = { (std::max)(mesh.LocalMax.X, p.X), (std::max)(mesh.LocalMax.Y, p.Y), (std::max)(mesh.LocalMax.Z, p.Z) };
        }
    }

    // Objects with a turn around Z and a tilt.
    SceneObjects objects{};
    for (std::uint32_t handle{ 1 }; handle <= 6; ++handle)
    {
        const float angle{ unit(random) * 3.1f };
        const float tilt{ unit(random) * 0.6f };
        const float cz{ std::cos(angle) }, sz{ std::sin(angle) }, cx{ std::cos(tilt) }, sx{ std::sin(tilt) };
        const Vec3 where{ point(6.0f) };
        const std::array<float, 12> transform{
            cz, -sz * cx, sz * sx, where.X,
            sz, cz * cx, -cz * sx, where.Y,
            0.0f, sx, cx, where.Z };

        objects.push_back(MakeObject(handle, mesh, transform, handle % 2 == 0));
    }

    RaycastScene scene{};
    Arm(scene, objects);

    int hits{ 0 };
    for (int index{ 0 }; index < 2000; ++index)
    {
        const Vec3 origin{ point(14.0f) };
        Vec3 direction{ unit(random), unit(random), unit(random) };
        const float length{ std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z) };
        direction = { direction.X / length, direction.Y / length, direction.Z / length };

        const RaycastHit expected{ CastLinear(objects, origin, direction, 30.0f) };
        RaycastHit actual{};
        CastScene(scene, SelfExclusion{}, origin, direction, 30.0f, actual, nullptr);

        ASSERT_EQ(actual.Hit, expected.Hit) << "ray " << index;
        if (!expected.Hit) continue;

        ++hits;
        EXPECT_EQ(actual.Distance, expected.Distance) << "ray " << index;
        EXPECT_EQ(actual.ObjectHandle, expected.ObjectHandle) << "ray " << index;
        EXPECT_EQ(actual.RegionIndex, expected.RegionIndex) << "ray " << index;
        EXPECT_EQ(actual.Point.X, expected.Point.X) << "ray " << index;
    }

    EXPECT_GT(hits, 100);
}

TEST(RaycastScene, CountsWhatItVisits)
{
    const CollMesh mesh{ MakeSquare() };
    const SceneObjects objects = Objects({ MakeObject(1, mesh, QuarterTurn(10.0f, 0.0f, 0.0f), false) });
    RaycastScene scene{};
    Arm(scene, objects);

    SceneCounters counters{};
    RaycastHit best{};
    CastScene(scene, SelfExclusion{}, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, k_Range, best, &counters);
    EXPECT_GE(counters.Nodes, 2u);
    EXPECT_GE(counters.Triangles, 1u);
}