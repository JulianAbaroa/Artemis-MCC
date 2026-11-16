#include <gtest/gtest.h>

import Blam.MapModel.World.System;
import Blam.MapModel.World.Type;
import std;

// Refit of the hierarchy over boxes (D302): the boxes a refit gives and what it leaves alone.

namespace
{
    using Blam::MapModel::World::System::BoundsBvhArea;
    using Blam::MapModel::World::System::BuildBoundsBvh;
    using Blam::MapModel::World::System::RefitBoundsBvh;
    using Blam::MapModel::World::Type::Raycast::BvhBounds;
    using Blam::MapModel::World::Type::Raycast::BvhNode;
    using Blam::MapModel::World::Type::Raycast::k_BoxPadding;

    // Boxes of 1 m around random centers.
    auto RandomBoxes(std::mt19937& random, std::size_t count, float spread) -> std::vector<BvhBounds>
    {
        std::uniform_real_distribution<float> unit{ -1.0f, 1.0f };
        std::vector<BvhBounds> boxes{};
        for (std::size_t index{ 0 }; index < count; ++index)
        {
            const float x{ unit(random) * spread };
            const float y{ unit(random) * spread };
            const float z{ unit(random) * spread };
            boxes.push_back(BvhBounds{ { x - 0.5f, y - 0.5f, z - 0.5f }, { x + 0.5f, y + 0.5f, z + 0.5f } });
        }

        return boxes;
    }

    // Whether every node holds what it should: a leaf the boxes of its slots plus the padding, an inner node the boxes of its children.
    auto IsFitted(const std::vector<BvhNode>& nodes, const std::vector<std::uint32_t>& order, const std::vector<BvhBounds>& boxes) -> bool
    {
        for (const BvhNode& node : nodes)
        {
            std::array<float, 3> low{ 1.0e30f, 1.0e30f, 1.0e30f };
            std::array<float, 3> high{ -1.0e30f, -1.0e30f, -1.0e30f };
            const auto include{ [&](const std::array<float, 3>& min, const std::array<float, 3>& max) {
                for (int axis{ 0 }; axis < 3; ++axis)
                {
                    low[axis] = (std::min)(low[axis], min[axis]);
                    high[axis] = (std::max)(high[axis], max[axis]);
                }
            } };

            float pad{ 0.0f };
            if (node.Count > 0)
            {
                pad = k_BoxPadding;
                for (std::int32_t slot{ node.Left }; slot < node.Left + node.Count; ++slot) include(boxes[order[static_cast<std::size_t>(slot)]].Min, boxes[order[static_cast<std::size_t>(slot)]].Max);
            }
            else
            {
                for (std::size_t child{ static_cast<std::size_t>(node.Left) }; child < static_cast<std::size_t>(node.Left) + 2; ++child) include(nodes[child].Min, nodes[child].Max);
            }

            for (int axis{ 0 }; axis < 3; ++axis)
            {
                if (node.Min[axis] != low[axis] - pad || node.Max[axis] != high[axis] + pad) return false;
            }
        }

        return true;
    }
}

TEST(BoundsBvhRefit, BoxesThatMoveGiveEveryNodeTheBoxItsContentsNeedAndKeepTheTopology)
{
    std::mt19937 random{ 302 };
    std::vector<BvhBounds> boxes{ RandomBoxes(random, 300, 20.0f) };
    std::vector<BvhNode> nodes{};
    std::vector<std::uint32_t> order{};
    BuildBoundsBvh(boxes, nodes, order);
    ASSERT_GT(nodes.size(), 10u);
    ASSERT_TRUE(IsFitted(nodes, order, boxes));

    const std::vector<BvhNode> before{ nodes };
    const std::vector<std::uint32_t> orderBefore{ order };
    for (int round{ 0 }; round < 5; ++round)
    {
        boxes = RandomBoxes(random, 300, 20.0f * static_cast<float>(round + 1));
        const float area{ RefitBoundsBvh(boxes, order, nodes) };

        EXPECT_TRUE(IsFitted(nodes, order, boxes)) << "round " << round;
        EXPECT_EQ(area, BoundsBvhArea(nodes));
        EXPECT_EQ(order, orderBefore);
        ASSERT_EQ(nodes.size(), before.size());
        for (std::size_t index{ 0 }; index < nodes.size(); ++index)
        {
            EXPECT_EQ(nodes[index].Left, before[index].Left);
            EXPECT_EQ(nodes[index].Count, before[index].Count);
        }
    }

    // The boxes spread 5 times as much, so the hierarchy built for the first ones is worse for them.
    EXPECT_GT(BoundsBvhArea(nodes), BoundsBvhArea(before) * 2.0f);
}

TEST(BoundsBvhRefit, RefitOfTheSameBoxesChangesNothing)
{
    std::mt19937 random{ 7 };
    const std::vector<BvhBounds> boxes{ RandomBoxes(random, 120, 10.0f) };
    std::vector<BvhNode> nodes{};
    std::vector<std::uint32_t> order{};
    BuildBoundsBvh(boxes, nodes, order);

    const std::vector<BvhNode> built{ nodes };
    RefitBoundsBvh(boxes, order, nodes);
    for (std::size_t index{ 0 }; index < nodes.size(); ++index)
    {
        EXPECT_EQ(nodes[index].Min, built[index].Min) << "node " << index;
        EXPECT_EQ(nodes[index].Max, built[index].Max) << "node " << index;
    }
}

TEST(BoundsBvhRefit, OneBoxIsALeafAndNoBoxesIsAnEmptyHierarchy)
{
    std::vector<BvhNode> nodes{};
    std::vector<std::uint32_t> order{};
    EXPECT_EQ(RefitBoundsBvh({}, order, nodes), 0.0f);
    EXPECT_TRUE(nodes.empty());

    std::vector<BvhBounds> boxes{ BvhBounds{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } } };
    BuildBoundsBvh(boxes, nodes, order);
    ASSERT_EQ(nodes.size(), 1u);

    boxes[0] = BvhBounds{ { 5.0f, 6.0f, 7.0f }, { 6.0f, 8.0f, 10.0f } };
    EXPECT_EQ(RefitBoundsBvh(boxes, order, nodes), 0.0f);
    EXPECT_EQ(nodes[0].Min, (std::array<float, 3>{ 5.0f - k_BoxPadding, 6.0f - k_BoxPadding, 7.0f - k_BoxPadding }));
    EXPECT_EQ(nodes[0].Max, (std::array<float, 3>{ 6.0f + k_BoxPadding, 8.0f + k_BoxPadding, 10.0f + k_BoxPadding }));
    EXPECT_EQ(nodes[0].Count, 1);
}

TEST(BoundsBvhRefit, BoxesThatDoNotMatchTheHierarchyLeaveItAlone)
{
    std::mt19937 random{ 11 };
    const std::vector<BvhBounds> boxes{ RandomBoxes(random, 50, 10.0f) };
    std::vector<BvhNode> nodes{};
    std::vector<std::uint32_t> order{};
    BuildBoundsBvh(boxes, nodes, order);

    const std::vector<BvhNode> built{ nodes };
    const std::vector<BvhBounds> fewer{ RandomBoxes(random, 49, 10.0f) };
    RefitBoundsBvh(fewer, order, nodes);
    for (std::size_t index{ 0 }; index < nodes.size(); ++index) EXPECT_EQ(nodes[index].Min, built[index].Min);
}