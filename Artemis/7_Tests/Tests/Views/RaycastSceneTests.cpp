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
// a linear reference written here. RS.1 (D299): the reuse of the parts of fixed objects, the store between ticks and the Self update.

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
    using Views::Raycast::Type::FixedPartsCache;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneKeyFunction;
    using Views::Raycast::Type::SceneObject;
    using Views::Raycast::Type::SceneObjects;
    using Views::Raycast::Type::ScenePartsFunction;
    using Views::Raycast::Type::SceneRefs;
    using Views::Raycast::Type::SelfExclusion;

    constexpr float k_Range{ 50.0f };

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

    auto MakeObject(std::uint32_t handle, const CollMesh& mesh, const std::array<float, 12>& transform, bool isFixed) -> SceneObject
    {
        CollidablePart part{};
        part.Source = &mesh;
        part.Transform = transform;
        return SceneObject{ handle, isFixed, { part } };
    }

    auto Objects(std::initializer_list<SceneObject> list) -> SceneObjects
    {
        SceneObjects objects{};
        for (const SceneObject& object : list) objects.push_back(object);
        return objects;
    }

    auto Arm(RaycastScene& scene, const SceneObjects& objects) -> void
    {
        ArmFixedScene(scene, objects);
        ArmMobileScene(scene, objects);
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

TEST(RaycastScene, FixedTreeRebuildsOnlyWhenTheFixedPartsOrTheirPosesChange)
{
    const CollMesh mesh{ MakeSquare() };
    SceneObjects objects = Objects({ MakeObject(1, mesh, QuarterTurn(0.0f, 0.0f, 0.0f), true), MakeObject(2, mesh, QuarterTurn(5.0f, 0.0f, 0.0f), false) });
    RaycastScene scene{};

    EXPECT_TRUE(ArmFixedScene(scene, objects));
    EXPECT_FALSE(ArmFixedScene(scene, objects));
    EXPECT_EQ(scene.FixedBuilds, 1u);
    EXPECT_EQ(scene.Fixed.Pieces.size(), 1u);

    // A mobile object that moves or leaves does not touch the fixed tree.
    objects[1].Parts[0].Transform = QuarterTurn(6.0f, 1.0f, 0.0f);
    EXPECT_FALSE(ArmFixedScene(scene, objects));
    ArmMobileScene(scene, objects);
    EXPECT_EQ(scene.Mobile.Pieces.size(), 1u);
    objects.pop_back();
    EXPECT_FALSE(ArmFixedScene(scene, objects));

    // One entry of one matrix is enough.
    objects[0].Parts[0].Transform[3] = 0.001f;
    EXPECT_TRUE(ArmFixedScene(scene, objects));
    EXPECT_EQ(scene.FixedBuilds, 2u);

    // A new fixed object, and one that leaves.
    objects.push_back(MakeObject(9, mesh, QuarterTurn(0.0f, 8.0f, 0.0f), true));
    EXPECT_TRUE(ArmFixedScene(scene, objects));
    EXPECT_EQ(scene.Fixed.Pieces.size(), 2u);
    objects.pop_back();
    EXPECT_TRUE(ArmFixedScene(scene, objects));
    EXPECT_EQ(scene.FixedBuilds, 4u);
    EXPECT_EQ(scene.Meshes.size(), 1u);
}

TEST(RaycastScene, StoreCleanupMarksAndTheNextCallForgetsMeshesTreesAndParts)
{
    const CollMesh mesh{ MakeSquare() };
    const SceneObjects objects = Objects({ MakeObject(1, mesh, QuarterTurn(10.0f, 0.0f, 0.0f), true), MakeObject(2, mesh, QuarterTurn(20.0f, 0.0f, 0.0f), false) });
    RaycastSceneStore store{};
    Arm(store.Scene(), objects);
    store.Parts().emplace(1u, Views::Raycast::Type::FixedParts{});
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
    EXPECT_TRUE(store.Parts().empty());
    EXPECT_FALSE(Cast(scene, SelfExclusion{}, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).Hit);

    // It arms again from nothing, and a second call does not forget again.
    EXPECT_TRUE(ArmFixedScene(scene, objects));
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
        FixedPartsCache Reuse{};

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
            const SceneKeyFunction keyOf{ [&](std::uint32_t handle, CollidablePartsKey& key, const CollidablePartsKey*)
            {
                const auto found{ Keys.find(handle) };
                if (found == Keys.end()) return false;
                key = found->second;
                return true;
            } };
            const ScenePartsFunction partsOf{ [&](std::uint32_t handle) -> std::optional<std::vector<CollidablePart>>
            {
                ++PartsCalls[handle];
                CollidablePart part{};
                part.Source = &Mesh;
                part.Transform = Poses[handle];
                return std::vector<CollidablePart>{ part };
            } };

            return GatherSceneObjects(Reuse, Refs, keyOf, partsOf, &reused);
        }
    };
}

TEST(RaycastScene, ASameKeyReusesThePartsOfAFixedObjectWithoutCollectingThem)
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
    EXPECT_EQ(reused, 2u);
    EXPECT_EQ(fixture.PartsCalls[5], 1);
    EXPECT_EQ(fixture.PartsCalls[2], 1);

    // The mobile object is always collected, and the result is the one of collecting everything.
    EXPECT_EQ(fixture.PartsCalls[9], 2);
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

TEST(RaycastScene, AnObjectThatIsGoneOrNotFixedAnymoreLeavesTheReuse)
{
    GatherFixture fixture{};
    fixture.Add(1, true, 1.0f);
    fixture.Add(2, true, 2.0f);
    fixture.Add(3, true, 3.0f);

    std::size_t reused{};
    fixture.Gather(reused);
    ASSERT_EQ(fixture.Reuse.size(), 3u);

    // Handle 1 disappears and 2 stops being fixed.
    fixture.Refs.erase(fixture.Refs.begin());
    fixture.Refs[0].IsFixed = false;
    fixture.Gather(reused);
    EXPECT_EQ(reused, 1u);
    EXPECT_EQ(fixture.Reuse.size(), 1u);
    EXPECT_TRUE(fixture.Reuse.contains(3u));

    // Handle 1 comes back (a new object with the same handle) and is collected again.
    fixture.Refs.push_back({ 1, true });
    fixture.Gather(reused);
    EXPECT_EQ(reused, 1u);
    EXPECT_EQ(fixture.PartsCalls[1], 2);
}

TEST(RaycastScene, AFixedObjectWithoutPartsIsNotRecollectedEither)
{
    GatherFixture fixture{};
    fixture.Add(1, true, 1.0f);
    const SceneKeyFunction keyOf{ [&](std::uint32_t, CollidablePartsKey& key, const CollidablePartsKey*) { key = fixture.Keys[1]; return true; } };
    int calls{ 0 };
    const ScenePartsFunction none{ [&](std::uint32_t) -> std::optional<std::vector<CollidablePart>> { ++calls; return std::vector<CollidablePart>{}; } };

    std::size_t reused{};
    EXPECT_TRUE(GatherSceneObjects(fixture.Reuse, fixture.Refs, keyOf, none, &reused).empty());
    EXPECT_TRUE(GatherSceneObjects(fixture.Reuse, fixture.Refs, keyOf, none, &reused).empty());
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(reused, 1u);
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

    // The wall is mobile here: nothing of it is kept between ticks.
    EXPECT_TRUE(m_Egocentric.m_RaycastSceneStore.Parts().empty());
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

    // The object moves: the key changes, and the fixed tree is built again.
    this->PublishWall(66.0f, Physics::Fixed);
    EXPECT_NEAR(this->Aim().Distance, 46.0f, 1.0e-3f);
    EXPECT_EQ(m_Egocentric.m_RaycastSceneStore.Scene().FixedBuilds, 2u);
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