#include <gtest/gtest.h>

import Common.Math.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.System;
import Blam.MapModel.World.Type;
import std;

// The static ray of the map (RK.1, RV.1): the guards of Raycaster::Cast, the tie between two static triangles and RayEntersNode against a
// scalar reference over random boxes and rays and the cases that were only reasoned by hand (rays with components at zero, tiny components,
// a ray that enters the box exactly at tMax, a box of no volume, the last node of a vector).

namespace
{
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::State::DefinitionsStore;
    using Blam::MapModel::Definitions::Type::Sbsp::Sbsp;
    using Blam::MapModel::World::System::InverseDirection;
    using Blam::MapModel::World::System::RayEntersNode;
    using Blam::MapModel::World::System::Raycaster;
    using Blam::MapModel::World::Type::Raycast::BvhNode;
    using Blam::MapModel::World::Type::Raycast::RayLanes;

    constexpr float k_NaN{ std::numeric_limits<float>::quiet_NaN() };
    constexpr float k_Infinity{ std::numeric_limits<float>::infinity() };

    // Raycaster over the collision triangles of one sbsp.
    auto Build(Raycaster& raycaster, DefinitionsStore& store, const std::vector<Triangle>& triangles) -> void
    {
        Sbsp sbsp{};
        sbsp.TagName = "test";
        sbsp.CollisionGeometry = triangles;
        store.m_Sbsp.Add("test", std::move(sbsp));
        store.m_Sbsp.Freeze();
        raycaster.Build(store);
    }

    // A square of 2 x 2 m at the height z, facing up, as two triangles.
    auto Floor(float z, std::int16_t material) -> std::vector<Triangle>
    {
        return {
            Triangle{ { -1.0f, -1.0f, z }, { 1.0f, -1.0f, z }, { 1.0f, 1.0f, z }, 0, material },
            Triangle{ { -1.0f, -1.0f, z }, { 1.0f, 1.0f, z }, { -1.0f, 1.0f, z }, 0, material } };
    }

    auto MakeLanes(const Vec3& origin, const Vec3& direction) -> RayLanes
    {
        return RayLanes{ { origin.X, origin.Y, origin.Z, 0.0f },
            { InverseDirection(direction.X), InverseDirection(direction.Y), InverseDirection(direction.Z), 0.0f } };
    }

    // The slabs one axis at a time, with the same operations RayEntersNode does for the three together.
    auto Reference(const BvhNode& node, const RayLanes& ray, float tMax, float& enter) -> bool
    {
        float t0{ 0.0f };
        float t1{ tMax };
        for (std::size_t axis{ 0 }; axis < 3; ++axis)
        {
            const float ta{ (node.Min[axis] - ray.Origin[axis]) * ray.Inverse[axis] };
            const float tb{ (node.Max[axis] - ray.Origin[axis]) * ray.Inverse[axis] };
            t0 = (std::max)(t0, (std::min)(ta, tb));
            t1 = (std::min)(t1, (std::max)(ta, tb));
        }

        if (t0 > t1) return false;

        enter = t0;
        return true;
    }

    auto MakeNode(const Vec3& min, const Vec3& max) -> BvhNode
    {
        BvhNode node{};
        node.Min = { min.X, min.Y, min.Z };
        node.Max = { max.X, max.Y, max.Z };
        return node;
    }

    // Checks RayEntersNode against the reference for one case.
    auto ExpectSame(const BvhNode& node, const Vec3& origin, const Vec3& direction, float tMax, std::string_view what) -> bool
    {
        const RayLanes lanes{ MakeLanes(origin, direction) };
        float expectedEnter{ -1.0f };
        float actualEnter{ -1.0f };
        const bool expected{ Reference(node, lanes, tMax, expectedEnter) };
        const bool actual{ RayEntersNode(node, lanes, tMax, actualEnter) };
        EXPECT_EQ(actual, expected) << what;
        if (actual && expected) EXPECT_FLOAT_EQ(actualEnter, expectedEnter) << what;
        return actual;
    }
}

TEST(RaycasterTests, NonFiniteRayMisses)
{
    Raycaster raycaster{};
    DefinitionsStore store{};
    Build(raycaster, store, Floor(0.0f, 4));

    const Vec3 origin{ 0.0f, 0.0f, 5.0f };
    const Vec3 down{ 0.0f, 0.0f, -1.0f };
    const auto hit{ raycaster.Cast(origin, down, 100.0f) };
    ASSERT_TRUE(hit.IsHit);
    EXPECT_FLOAT_EQ(hit.Distance, 5.0f);

    const std::array<float, 2> bad{ k_NaN, k_Infinity };
    for (const float value : bad)
    {
        EXPECT_FALSE(raycaster.Cast({ value, 0.0f, 5.0f }, down, 100.0f).IsHit) << value;
        EXPECT_FALSE(raycaster.Cast({ 0.0f, value, 5.0f }, down, 100.0f).IsHit) << value;
        EXPECT_FALSE(raycaster.Cast({ 0.0f, 0.0f, value }, down, 100.0f).IsHit) << value;
        EXPECT_FALSE(raycaster.Cast(origin, { value, 0.0f, -1.0f }, 100.0f).IsHit) << value;
        EXPECT_FALSE(raycaster.Cast(origin, { 0.0f, value, -1.0f }, 100.0f).IsHit) << value;
        EXPECT_FALSE(raycaster.Cast(origin, { 0.0f, 0.0f, value }, 100.0f).IsHit) << value;
        EXPECT_FALSE(raycaster.Cast(origin, down, value).IsHit) << value;
    }

    // A miss leaves the hit empty, with no distance that a caller could mistake for a number.
    const auto nan{ raycaster.Cast(origin, { k_NaN, k_NaN, k_NaN }, 100.0f) };
    EXPECT_FALSE(nan.IsHit);
    EXPECT_EQ(nan.Distance, 0.0f);
}

TEST(RaycasterTests, StaticTieKeepsTheFirstTriangleOfTheBuild)
{
    // Two coplanar triangles with the same shape and different material and flags: the ray reaches both at the same distance.
    std::vector<Triangle> triangles{};
    triangles.push_back(Triangle{ { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, 1, 11 });
    triangles.push_back(Triangle{ { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, 2, 22 });

    Raycaster raycaster{};
    DefinitionsStore store{};
    Build(raycaster, store, triangles);

    const auto hit{ raycaster.Cast({ 0.5f, -0.25f, 3.0f }, { 0.0f, 0.0f, -1.0f }, 10.0f) };
    ASSERT_TRUE(hit.IsHit);
    EXPECT_FLOAT_EQ(hit.Distance, 3.0f);
    EXPECT_EQ(hit.Material, 11);
    EXPECT_EQ(hit.SurfaceFlags, 1);

    // Seen from below the first one still wins, and the order of the sbsp is the one that counts.
    const auto below{ raycaster.Cast({ 0.5f, -0.25f, -3.0f }, { 0.0f, 0.0f, 1.0f }, 10.0f) };
    ASSERT_TRUE(below.IsHit);
    EXPECT_EQ(below.Material, 11);

    std::reverse(triangles.begin(), triangles.end());
    Raycaster reversed{};
    DefinitionsStore reversedStore{};
    Build(reversed, reversedStore, triangles);
    EXPECT_EQ(reversed.Cast({ 0.5f, -0.25f, 3.0f }, { 0.0f, 0.0f, -1.0f }, 10.0f).Material, 22);
}

TEST(RaycasterBoxTests, RandomBoxesAndRaysGiveTheSameAnswerAsTheScalarReference)
{
    std::mt19937 random{ 303 };
    std::uniform_real_distribution<float> unit{ -1.0f, 1.0f };
    std::uniform_real_distribution<float> positive{ 0.0f, 1.0f };
    std::uniform_int_distribution<int> pick{ 0, 9 };

    std::size_t entered{ 0 };
    constexpr int k_Cases{ 100000 };
    for (int index{ 0 }; index < k_Cases; ++index)
    {
        const Vec3 min{ unit(random) * 20.0f, unit(random) * 20.0f, unit(random) * 20.0f };
        const float extentX{ pick(random) == 0 ? 0.0f : positive(random) * 10.0f };
        const float extentY{ pick(random) == 0 ? 0.0f : positive(random) * 10.0f };
        const float extentZ{ pick(random) == 0 ? 0.0f : positive(random) * 10.0f };
        const BvhNode node{ MakeNode(min, { min.X + extentX, min.Y + extentY, min.Z + extentZ }) };

        // Origins on the faces of the box now and then, where the slab distances are zero.
        Vec3 origin{ unit(random) * 25.0f, unit(random) * 25.0f, unit(random) * 25.0f };
        if (pick(random) == 0) origin.X = node.Min[0];
        if (pick(random) == 0) origin.Y = node.Max[1];
        if (pick(random) == 0) origin.Z = node.Min[2];

        Vec3 direction{ unit(random), unit(random), unit(random) };
        const float length{ std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z) };
        if (length < 1.0e-3f) continue;

        direction = { direction.X / length, direction.Y / length, direction.Z / length };

        // Components at zero or too small to invert, on one or two axes.
        const std::array<float, 3> flat{ 0.0f, 1.0e-30f, -1.0e-30f };
        if (pick(random) < 3) direction.X = flat[static_cast<std::size_t>(pick(random)) % 3];
        if (pick(random) < 2) direction.Y = flat[static_cast<std::size_t>(pick(random)) % 3];
        if (pick(random) == 0) direction.Z = flat[static_cast<std::size_t>(pick(random)) % 3];

        const float tMax{ positive(random) * 60.0f };
        if (ExpectSame(node, origin, direction, tMax, "random case")) ++entered;
        if (::testing::Test::HasFailure()) FAIL() << "case " << index;
    }

    EXPECT_GT(entered, 500u);
}

TEST(RaycasterBoxTests, ARayThroughAPointOfTheBoxEntersIt)
{
    std::mt19937 random{ 304 };
    std::uniform_real_distribution<float> unit{ -1.0f, 1.0f };
    std::uniform_real_distribution<float> inside{ 0.1f, 0.9f };

    int checked{ 0 };
    for (int index{ 0 }; index < 50000; ++index)
    {
        const Vec3 min{ unit(random) * 20.0f, unit(random) * 20.0f, unit(random) * 20.0f };
        const Vec3 size{ 0.2f + std::abs(unit(random)) * 8.0f, 0.2f + std::abs(unit(random)) * 8.0f, 0.2f + std::abs(unit(random)) * 8.0f };
        const BvhNode node{ MakeNode(min, { min.X + size.X, min.Y + size.Y, min.Z + size.Z }) };

        const Vec3 point{ min.X + size.X * inside(random), min.Y + size.Y * inside(random), min.Z + size.Z * inside(random) };
        const Vec3 origin{ unit(random) * 30.0f, unit(random) * 30.0f, unit(random) * 30.0f };
        Vec3 direction{ point.X - origin.X, point.Y - origin.Y, point.Z - origin.Z };
        const float distance{ std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z) };
        if (distance < 1.0e-2f) continue;

        direction = { direction.X / distance, direction.Y / distance, direction.Z / distance };

        float enter{ -1.0f };
        ASSERT_TRUE(RayEntersNode(node, MakeLanes(origin, direction), distance + 0.01f, enter)) << "case " << index;
        EXPECT_LE(enter, distance + 0.01f) << "case " << index;
        ++checked;
    }

    EXPECT_GT(checked, 49000);
}

TEST(RaycasterBoxTests, RaysWithComponentsAtZeroKeepTheSlabOfThatAxis)
{
    const BvhNode box{ MakeNode({ 4.0f, -1.0f, -1.0f }, { 6.0f, 1.0f, 1.0f }) };
    const Vec3 along{ 1.0f, 0.0f, 0.0f };

    // One component at zero: the origin inside the slab of that axis, outside it, and exactly on its min and max faces.
    float enter{ -1.0f };
    EXPECT_TRUE(RayEntersNode(box, MakeLanes({ 0.0f, 0.0f, 0.0f }, along), 100.0f, enter));
    EXPECT_FLOAT_EQ(enter, 4.0f);
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, 1.5f, 0.0f }, along), 100.0f, enter));
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, -1.5f, 0.0f }, along), 100.0f, enter));
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, 0.0f, 3.0f }, along), 100.0f, enter));
    EXPECT_TRUE(RayEntersNode(box, MakeLanes({ 0.0f, -1.0f, 0.0f }, along), 100.0f, enter));
    EXPECT_TRUE(RayEntersNode(box, MakeLanes({ 0.0f, 0.0f, -1.0f }, along), 100.0f, enter));

    // On the max face the slab ends at 0, so the ray is out before it reaches the box. Nothing a leaf holds is there, because a box is
    // padded by k_BoxPadding around its triangles. The test fixes it so a change of the slab test shows.
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, 1.0f, 0.0f }, along), 100.0f, enter));
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, 0.0f, 1.0f }, along), 100.0f, enter));

    // Two components at zero.
    EXPECT_TRUE(RayEntersNode(box, MakeLanes({ 0.0f, 0.5f, -0.5f }, along), 100.0f, enter));
    EXPECT_TRUE(RayEntersNode(box, MakeLanes({ 0.0f, -1.0f, -1.0f }, along), 100.0f, enter));
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, -1.0f, 1.0f }, along), 100.0f, enter));
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, -1.5f, 1.0f }, along), 100.0f, enter));
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, 0.5f, 1.5f }, along), 100.0f, enter));

    // A ray that starts inside the box enters it at once, one that points away from it does not.
    EXPECT_TRUE(RayEntersNode(box, MakeLanes({ 5.0f, 0.0f, 0.0f }, along), 100.0f, enter));
    EXPECT_EQ(enter, 0.0f);
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }), 100.0f, enter));

    // The three components at zero the way the traversal could get them: only an origin inside the box is in it.
    EXPECT_TRUE(RayEntersNode(box, MakeLanes({ 5.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }), 100.0f, enter));
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }), 100.0f, enter));

    // A component of 1e-30 is inverted as one of 1e-20 or less: the same as zero.
    EXPECT_EQ(InverseDirection(1.0e-30f), InverseDirection(0.0f));
    EXPECT_EQ(InverseDirection(-1.0e-30f), InverseDirection(0.0f));
    EXPECT_TRUE(RayEntersNode(box, MakeLanes({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0e-30f, -1.0e-30f }), 100.0f, enter));
    EXPECT_FALSE(RayEntersNode(box, MakeLanes({ 0.0f, 3.0f, 0.0f }, { 1.0f, 1.0e-30f, 0.0f }), 100.0f, enter));
    ExpectSame(box, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0e-30f, -1.0e-30f }, 100.0f, "tiny components");
}

TEST(RaycasterBoxTests, TheBoxThatStartsExactlyAtTMaxIsEnteredAndOneBeyondIsNot)
{
    const BvhNode box{ MakeNode({ 5.0f, -1.0f, -1.0f }, { 7.0f, 1.0f, 1.0f }) };
    const RayLanes lanes{ MakeLanes({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }) };

    float enter{ -1.0f };
    EXPECT_TRUE(RayEntersNode(box, lanes, 5.0f, enter));
    EXPECT_EQ(enter, 5.0f);
    EXPECT_FALSE(RayEntersNode(box, lanes, 4.999f, enter));
    EXPECT_TRUE(RayEntersNode(box, lanes, 5.001f, enter));

    // A box behind the origin is never entered, whatever the range.
    const BvhNode behind{ MakeNode({ -7.0f, -1.0f, -1.0f }, { -5.0f, 1.0f, 1.0f }) };
    EXPECT_FALSE(RayEntersNode(behind, lanes, 1.0e9f, enter));

    // And one that touches the origin is entered at 0.
    const BvhNode touching{ MakeNode({ -2.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f }) };
    EXPECT_TRUE(RayEntersNode(touching, lanes, 10.0f, enter));
    EXPECT_EQ(enter, 0.0f);
}

TEST(RaycasterBoxTests, ABoxOfNoVolumeAnswersAsTheReferenceAndTheBoxOfAHierarchyAlwaysHasVolume)
{
    std::mt19937 random{ 305 };
    std::uniform_real_distribution<float> unit{ -1.0f, 1.0f };

    for (int index{ 0 }; index < 20000; ++index)
    {
        const Vec3 point{ unit(random) * 10.0f, unit(random) * 10.0f, unit(random) * 10.0f };
        const BvhNode node{ MakeNode(point, point) };
        Vec3 direction{ unit(random), unit(random), unit(random) };
        const float length{ std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z) };
        if (length < 1.0e-3f) continue;

        direction = { direction.X / length, direction.Y / length, direction.Z / length };
        ExpectSame(node, { unit(random) * 15.0f, unit(random) * 15.0f, unit(random) * 15.0f }, direction, 40.0f, "point box");
        if (::testing::Test::HasFailure()) FAIL() << "case " << index;
    }

    // A ray along an axis through the point of a box of no volume does not enter it (the slabs of the other axes are empty): this is why the
    // boxes of the hierarchies carry k_BoxPadding.
    float enter{ -1.0f };
    const BvhNode flat{ MakeNode({ 5.0f, 0.0f, 0.0f }, { 5.0f, 0.0f, 0.0f }) };
    EXPECT_FALSE(RayEntersNode(flat, MakeLanes({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }), 100.0f, enter));
    const BvhNode padded{ MakeNode({ 5.0f - 1.0e-3f, -1.0e-3f, -1.0e-3f }, { 5.0f + 1.0e-3f, 1.0e-3f, 1.0e-3f }) };
    EXPECT_TRUE(RayEntersNode(padded, MakeLanes({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }), 100.0f, enter));
}

TEST(RaycasterBoxTests, TheLastNodeOfAVectorAnswersLikeTheReferenceWhateverItsLeftHolds)
{
    // The vector holds exactly the nodes: the 16 byte read of the upper corner of the last one ends inside the node, on its Left.
    std::vector<BvhNode> nodes{};
    nodes.reserve(3);
    nodes.push_back(MakeNode({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }));
    nodes.push_back(MakeNode({ 3.0f, -1.0f, -1.0f }, { 5.0f, 1.0f, 1.0f }));
    nodes.push_back(MakeNode({ 8.0f, -1.0f, -1.0f }, { 9.0f, 1.0f, 1.0f }));
    nodes.shrink_to_fit();
    ASSERT_EQ(nodes.capacity(), nodes.size());

    // Values that are not numbers or are huge as a float, in the bytes that follow the upper corner.
    const std::array<std::int32_t, 4> lefts{ 0, 1, 0x7fc00000, -1 };
    const RayLanes lanes{ MakeLanes({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }) };
    for (const std::int32_t left : lefts)
    {
        nodes.back().Left = left;
        float enter{ -1.0f };
        EXPECT_TRUE(RayEntersNode(nodes.back(), lanes, 20.0f, enter)) << left;
        EXPECT_EQ(enter, 8.0f) << left;
        EXPECT_FALSE(RayEntersNode(nodes.back(), lanes, 7.0f, enter)) << left;
        ExpectSame(nodes.back(), { 0.0f, 0.5f, 0.0f }, { 0.8f, 0.0f, 0.6f }, 20.0f, "last node");
    }
}