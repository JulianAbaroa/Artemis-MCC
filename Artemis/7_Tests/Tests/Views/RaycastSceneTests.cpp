#include <gtest/gtest.h>

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Perception.Collidable.Type;
import Views.Raycast.Type;
import Views.Raycast.System;
import std;

// The shared two level ray scene (D298, RS.0) with made-up meshes: poses, exclusion, ties, the fixed tree and the exactness against
// the linear cast of the dynamic cache.

namespace
{
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using Perception::Collidable::Type::CollidablePart;
    using Views::Raycast::System::ArmFixedScene;
    using Views::Raycast::System::ArmMobileScene;
    using Views::Raycast::System::CastDynamic;
    using Views::Raycast::System::CastScene;
    using Views::Raycast::System::CleanupScene;
    using Views::Raycast::System::PrepareParts;
    using Views::Raycast::Type::CachedDynamic;
    using Views::Raycast::Type::DynamicCache;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneObject;
    using Views::Raycast::Type::SceneObjects;
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

TEST(RaycastScene, CleanupForgetsMeshesAndTrees)
{
    const CollMesh mesh{ MakeSquare() };
    const SceneObjects objects = Objects({ MakeObject(1, mesh, QuarterTurn(10.0f, 0.0f, 0.0f), true), MakeObject(2, mesh, QuarterTurn(20.0f, 0.0f, 0.0f), false) });
    RaycastScene scene{};
    Arm(scene, objects);
    ASSERT_TRUE(Cast(scene, SelfExclusion{}, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).Hit);

    CleanupScene(scene);
    EXPECT_TRUE(scene.Meshes.empty());
    EXPECT_TRUE(scene.Fixed.Pieces.empty());
    EXPECT_TRUE(scene.Mobile.Pieces.empty());
    EXPECT_EQ(scene.FixedBuilds, 0u);
    EXPECT_FALSE(Cast(scene, SelfExclusion{}, { 10.0f, -5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }).Hit);

    // It arms again from nothing.
    EXPECT_TRUE(ArmFixedScene(scene, objects));
}

TEST(RaycastScene, GivesTheSameHitAsTheLinearCastOnRandomRays)
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

    // Objects with a turn around Z and a tilt, in the cache of the linear cast and in the scene.
    SceneObjects objects{};
    DynamicCache cache{};
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

        CachedDynamic entry{};
        entry.Parts = PrepareParts(objects.back().Parts);
        entry.BoundingRadius = 1.0e6f;
        cache.emplace(handle, std::move(entry));
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

        RaycastHit expected{};
        CastDynamic(cache, origin, direction, 30.0f, expected);
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