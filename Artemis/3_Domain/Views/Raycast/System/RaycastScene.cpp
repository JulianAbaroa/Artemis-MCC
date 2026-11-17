module;

#include <cmath>

module Views.Raycast.System;

import Common.Math.Type;
import Common.Math.System;
import Common.Geometry.System;
import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.Type;
import Blam.Runtime.Object.Type;
import Perception.Collidable.Type;

namespace
{
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Physics;
    using Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using Blam::MapModel::World::Type::Raycast::BvhBounds;
    using Blam::MapModel::World::Type::Raycast::BvhNode;
    using Blam::MapModel::World::Type::Raycast::RayLanes;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::FixedRebuildRule;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::MeshBvh;
    using Views::Raycast::Type::MeshBvhs;
    using Views::Raycast::Type::MobileRebuildRule;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::SceneStamp;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneGatherCounters;
    using Views::Raycast::Type::SceneKeyFunction;
    using Views::Raycast::Type::SceneObject;
    using Views::Raycast::Type::SceneObjects;
    using Views::Raycast::Type::SceneReach;
    using Views::Raycast::Type::SceneReachPiece;
    using Views::Raycast::Type::SceneParts;
    using Views::Raycast::Type::ScenePartsCache;
    using Views::Raycast::Type::ScenePartsFunction;
    using Views::Raycast::Type::ScenePiece;
    using Views::Raycast::Type::SceneRef;
    using Views::Raycast::Type::SceneRefs;
    using Views::Raycast::Type::ScenePose;
    using Views::Raycast::Type::SceneTree;
    using Views::Raycast::Type::SelfExclusion;

    using Perception::Collidable::Type::CollidablePart;
    using Perception::Collidable::Type::CollidablePartsKey;
    using Perception::Collidable::System::CollidableService;
    using Blam::Runtime::Object::State::ObjectTableStore;
    using Blam::MapModel::World::System::Raycaster;

    using Blam::MapModel::World::Type::Raycast::k_BoxPadding;
    using Blam::MapModel::World::Type::Raycast::k_StackSize;
    using Views::Raycast::Type::Constant::k_RestGathers;

    // Slack added to the distance a piece may be from the origin of a reach, so a rounding at the border never leaves a piece out.
    constexpr float k_ReachMargin{ 0.01f };

    // Who wins between two dynamic hits at the same distance.
    struct TieKey
    {
        std::uint32_t Handle{ (std::numeric_limits<std::uint32_t>::max)() };
        std::uint32_t Part{ (std::numeric_limits<std::uint32_t>::max)() };
        std::uint32_t Triangle{ (std::numeric_limits<std::uint32_t>::max)() };

        auto operator<=>(const TieKey&) const = default;
    };

    // A ray in the space it is cast in, with what its traversals need.
    struct RayState
    {
        Vec3 Origin{};
        Vec3 Direction{};
        std::array<float, 3> Position{};
        std::array<float, 3> Inverse{};
        RayLanes Lanes{};
    };

    auto MakeRayState(const Vec3& origin, const Vec3& direction) -> RayState
    {
        RayState ray{};
        ray.Origin = origin;
        ray.Direction = direction;
        ray.Position = { origin.X, origin.Y, origin.Z };
        ray.Inverse = {
            Blam::MapModel::World::System::InverseDirection(direction.X),
            Blam::MapModel::World::System::InverseDirection(direction.Y),
            Blam::MapModel::World::System::InverseDirection(direction.Z) };
        ray.Lanes = RayLanes{ { ray.Position[0], ray.Position[1], ray.Position[2], 0.0f }, { ray.Inverse[0], ray.Inverse[1], ray.Inverse[2], 0.0f } };
        return ray;
    }

    // What a cast keeps while it goes through the trees.
    struct CastState
    {
        RaycastHit& Best;
        const SelfExclusion& Exclusion;
        const Vec3& WorldOrigin;
        const Vec3& WorldDirection;
        float MaxDistance{};
        SceneCounters* Counters{ nullptr };
        TieKey BestKey{};

        // Traversal stacks of the instance tree and of the mesh hierarchy inside it, set once per cast and not once per piece.
        std::array<std::int32_t, k_StackSize> TreeStack{};
        std::array<std::int32_t, k_StackSize> MeshStack{};

        auto Limit() const -> float
        {
            return Best.Hit ? Best.Distance : MaxDistance;
        }
    };

    // Pushes the children of a node that the ray enters, the nearer one last so it is visited first.
    auto PushChildren(const std::vector<BvhNode>& nodes, std::int32_t left, const RayState& ray, float limit,
        std::array<std::int32_t, k_StackSize>& stack, int& top) -> void
    {
        float leftEnter{};
        float rightEnter{};
        const bool hitLeft = Blam::MapModel::World::System::RayEntersNode(nodes[static_cast<std::size_t>(left)], ray.Lanes, limit, leftEnter);
        const bool hitRight = Blam::MapModel::World::System::RayEntersNode(nodes[static_cast<std::size_t>(left) + 1], ray.Lanes, limit, rightEnter);

        if (hitLeft && hitRight)
        {
            if (leftEnter <= rightEnter)
            {
                stack[top++] = left + 1;
                stack[top++] = left;
            }
            else
            {
                stack[top++] = left;
                stack[top++] = left + 1;
            }
        }
        else if (hitLeft)
        {
            stack[top++] = left;
        }
        else if (hitRight)
        {
            stack[top++] = left + 1;
        }
    }

    // Goes down the hierarchy of the mesh of a piece, in its local space.
    auto CastMesh(const ScenePiece& piece, const RayState& local, CastState& state) -> void
    {
        const MeshBvh& bvh{ *piece.Bvh };
        const std::vector<Triangle>& triangles{ piece.Part.Source->Triangles };
        if (bvh.Nodes.empty()) return;

        std::array<std::int32_t, k_StackSize>& stack{ state.MeshStack };
        int top{ 0 };
        stack[top++] = 0;

        while (top > 0)
        {
            const BvhNode& node = bvh.Nodes[static_cast<std::size_t>(stack[--top])];
            if (state.Counters) ++state.Counters->Nodes;

            if (node.Count > 0)
            {
                for (std::int32_t slot = node.Left; slot < node.Left + node.Count; ++slot)
                {
                    const std::uint32_t index{ bvh.Order[static_cast<std::size_t>(slot)] };
                    if (state.Counters) ++state.Counters->Triangles;

                    float distance{ 0.0f };
                    if (!Common::Geometry::System::RayIntersectsTriangle(local.Origin, local.Direction, triangles[index], state.Limit(), distance))
                    {
                        continue;
                    }

                    const TieKey key{ piece.Handle, piece.PartIndex, index };
                    const bool isNearer{ !state.Best.Hit || distance < state.Best.Distance };
                    const bool wonTie{ state.Best.Hit && distance == state.Best.Distance && state.Best.Kind == HitKind::Dynamic && key < state.BestKey };
                    if (!isNearer && !wonTie) continue;

                    state.Best.Hit = true;
                    state.Best.Kind = HitKind::Dynamic;
                    state.Best.Distance = distance;
                    state.Best.ObjectHandle = piece.Handle;
                    state.Best.RegionIndex = piece.Part.Source->RegionIndex;
                    state.Best.SurfaceFlags = 0;
                    state.Best.Material = -1;
                    state.Best.Point = {
                        state.WorldOrigin.X + state.WorldDirection.X * distance,
                        state.WorldOrigin.Y + state.WorldDirection.Y * distance,
                        state.WorldOrigin.Z + state.WorldDirection.Z * distance
                    };
                    state.BestKey = key;
                }

                continue;
            }

            PushChildren(bvh.Nodes, node.Left, local, state.Limit(), stack, top);
        }
    }

    // Takes the ray to the space of a piece and goes down its mesh if the ray crosses the box of the mesh.
    auto EnterPiece(const ScenePiece& piece, const RayState& world, float limit, CastState& state) -> void
    {
        RayState local{};
        Views::Raycast::System::PoseRay(piece.Part.ToLocal, world.Origin, world.Direction, local.Origin, local.Direction);
        local = MakeRayState(local.Origin, local.Direction);
        if (!Views::Raycast::System::RayHitsBox(local.Origin, local.Inverse, piece.Part.Source->LocalMin, piece.Part.Source->LocalMax, limit)) return;
        if (state.Counters) ++state.Counters->EnteredPieces;

        CastMesh(piece, local, state);
    }

    // Goes down one instance tree and into the mesh of every piece the ray reaches.
    // param filtered: Sorted handles whose pieces are skipped. Null for none.
    auto CastTree(const SceneTree& tree, const std::vector<std::uint32_t>* filtered, const RayState& world, CastState& state) -> void
    {
        if (tree.Nodes.empty()) return;

        std::array<std::int32_t, k_StackSize>& stack{ state.TreeStack };
        int top{ 0 };
        stack[top++] = 0;

        while (top > 0)
        {
            const BvhNode& node = tree.Nodes[static_cast<std::size_t>(stack[--top])];
            if (state.Counters)
            {
                ++state.Counters->Nodes;
                ++state.Counters->TreeNodes;
            }

            if (node.Count > 0)
            {
                for (std::int32_t slot = node.Left; slot < node.Left + node.Count; ++slot)
                {
                    const ScenePiece& piece{ tree.Pieces[tree.Order[static_cast<std::size_t>(slot)]] };
                    if (state.Counters) ++state.Counters->Pieces;

                    // The sphere test is arithmetic only, so it goes before the lookups of the exclusion and the filter.
                    const float limit{ state.Limit() };
                    if (!Views::Raycast::System::RayNearPoint(world.Origin, world.Direction, limit, piece.Part.Center, piece.Part.Radius)) continue;
                    if (state.Exclusion.contains(piece.Handle)) continue;
                    if (filtered && std::ranges::binary_search(*filtered, piece.Handle)) continue;
                    if (state.Counters) ++state.Counters->NearPieces;

                    EnterPiece(piece, world, limit, state);
                }

                continue;
            }

            PushChildren(tree.Nodes, node.Left, world, state.Limit(), stack, top);
        }
    }

    // The handles of an exclusion in a short array, to look at them without hashing. Larger sets stay in the exclusion.
    struct ExcludedHandles
    {
        std::array<std::uint32_t, 16> Handles{};
        std::size_t Count{ 0 };
        const SelfExclusion* Set{ nullptr };

        explicit ExcludedHandles(const SelfExclusion& exclusion)
        {
            if (exclusion.size() > Handles.size())
            {
                Set = &exclusion;
                return;
            }

            for (const std::uint32_t handle : exclusion) Handles[Count++] = handle;
        }

        auto Contains(std::uint32_t handle) const -> bool
        {
            if (Set) return Set->contains(handle);

            for (std::size_t index{ 0 }; index < Count; ++index)
            {
                if (Handles[index] == handle) return true;
            }

            return false;
        }
    };

    // Adds to reach the pieces of a tree that rays from its origin can touch, minus the excluded ones and the filtered ones.
    auto GatherTree(const SceneTree& tree, const std::vector<std::uint32_t>* filtered, const ExcludedHandles& exclusion, SceneReach& reach,
        SceneCounters* counters) -> void
    {
        if (tree.Nodes.empty()) return;

        const float radius{ reach.Radius };
        const std::array<float, 3> lo{ reach.Origin.X - radius, reach.Origin.Y - radius, reach.Origin.Z - radius };
        const std::array<float, 3> hi{ reach.Origin.X + radius, reach.Origin.Y + radius, reach.Origin.Z + radius };

        std::array<std::int32_t, k_StackSize> stack{};
        int top{ 0 };
        stack[top++] = 0;

        while (top > 0)
        {
            const BvhNode& node = tree.Nodes[static_cast<std::size_t>(stack[--top])];
            if (counters)
            {
                ++counters->Nodes;
                ++counters->TreeNodes;
            }

            if (node.Min[0] > hi[0] || node.Max[0] < lo[0] || node.Min[1] > hi[1] || node.Max[1] < lo[1] || node.Min[2] > hi[2] || node.Max[2] < lo[2]) continue;

            if (node.Count == 0)
            {
                stack[top++] = node.Left;
                stack[top++] = node.Left + 1;
                continue;
            }

            for (std::int32_t slot = node.Left; slot < node.Left + node.Count; ++slot)
            {
                const ScenePiece& piece{ tree.Pieces[tree.Order[static_cast<std::size_t>(slot)]] };
                const float dx{ piece.Part.Center.X - reach.Origin.X };
                const float dy{ piece.Part.Center.Y - reach.Origin.Y };
                const float dz{ piece.Part.Center.Z - reach.Origin.Z };
                const float limit{ radius + piece.Part.Radius + k_ReachMargin };
                if (dx * dx + dy * dy + dz * dz > limit * limit) continue;
                if (exclusion.Contains(piece.Handle)) continue;
                if (filtered && std::ranges::binary_search(*filtered, piece.Handle)) continue;

                const Vec3& boxMin{ piece.Part.Min };
                const Vec3& boxMax{ piece.Part.Max };
                reach.Pieces.push_back(SceneReachPiece{ &piece,
                    { boxMin.X - k_ReachMargin, boxMin.Y - k_ReachMargin, boxMin.Z - k_ReachMargin },
                    { boxMax.X + k_ReachMargin, boxMax.Y + k_ReachMargin, boxMax.Z + k_ReachMargin } });
            }
        }
    }

    // Sets the pieces of an instance tree to the parts of the objects whose IsCovered is covered, in place.
    // return: Whether they are the same pieces as before (handle, mesh and part index, in the same order), so only the poses changed.
    // note: The tree is not valid afterwards, its hierarchy is built or refitted by the caller.
    auto AssemblePieces(SceneTree& tree, const MeshBvhs& meshes, const SceneObjects& objects, bool covered) -> bool
    {
        std::size_t count{ 0 };
        bool isSame{ true };

        for (const SceneObject& object : objects)
        {
            if (object.IsCovered != covered) continue;

            std::uint32_t partIndex{ 0 };
            for (const auto& part : object.Parts)
            {
                ScenePiece piece{};
                if (!Views::Raycast::System::PreparePart(part, piece.Part)) continue;

                piece.Bvh = &meshes.at(piece.Part.Source);
                piece.Handle = object.Handle;
                piece.PartIndex = partIndex++;

                if (count >= tree.Pieces.size())
                {
                    isSame = false;
                    tree.Pieces.push_back(piece);
                }
                else
                {
                    const ScenePiece& armed{ tree.Pieces[count] };
                    if (armed.Handle != piece.Handle || armed.Bvh != piece.Bvh || armed.PartIndex != piece.PartIndex) isSame = false;
                    tree.Pieces[count] = piece;
                }

                ++count;
            }
        }

        if (count != tree.Pieces.size())
        {
            isSame = false;
            tree.Pieces.erase(tree.Pieces.begin() + static_cast<std::ptrdiff_t>(count), tree.Pieces.end());
        }

        return isSame;
    }

    // The box of each piece: the posed local box of its part cut with the box of its sphere. Both hold the part whatever its pose.
    auto PieceBounds(const std::vector<ScenePiece>& pieces) -> std::vector<BvhBounds>
    {
        std::vector<BvhBounds> bounds{};
        bounds.reserve(pieces.size());
        for (const ScenePiece& piece : pieces)
        {
            const Vec3& c{ piece.Part.Center };
            const float r{ piece.Part.Radius + k_BoxPadding };
            const Vec3& lo{ piece.Part.Min };
            const Vec3& hi{ piece.Part.Max };
            bounds.push_back(BvhBounds{
                { (std::max)(c.X - r, lo.X - k_BoxPadding), (std::max)(c.Y - r, lo.Y - k_BoxPadding), (std::max)(c.Z - r, lo.Z - k_BoxPadding) },
                { (std::min)(c.X + r, hi.X + k_BoxPadding), (std::min)(c.Y + r, hi.Y + k_BoxPadding), (std::min)(c.Z + r, hi.Z + k_BoxPadding) } });
        }

        return bounds;
    }

    // Builds the hierarchy of an instance tree from the boxes of its pieces.
    // return: The area of its inner nodes.
    auto BuildTree(SceneTree& tree, const std::vector<BvhBounds>& bounds) -> float
    {
        Blam::MapModel::World::System::BuildBoundsBvh(bounds, tree.Nodes, tree.Order);
        return Blam::MapModel::World::System::BoundsBvhArea(tree.Nodes);
    }

    // Builds an instance tree over the parts of the objects whose IsCovered is covered.
    auto ArmTree(SceneTree& tree, const MeshBvhs& meshes, const SceneObjects& objects, bool covered) -> void
    {
        AssemblePieces(tree, meshes, objects, covered);
        BuildTree(tree, PieceBounds(tree.Pieces));
    }

    auto BuildMeshBvh(const CollMesh& mesh) -> MeshBvh
    {
        std::vector<BvhBounds> bounds{};
        bounds.reserve(mesh.Triangles.size());
        for (const Triangle& tri : mesh.Triangles)
        {
            bounds.push_back(BvhBounds{
                { (std::min)({ tri.A.X, tri.B.X, tri.C.X }), (std::min)({ tri.A.Y, tri.B.Y, tri.C.Y }), (std::min)({ tri.A.Z, tri.B.Z, tri.C.Z }) },
                { (std::max)({ tri.A.X, tri.B.X, tri.C.X }), (std::max)({ tri.A.Y, tri.B.Y, tri.C.Y }), (std::max)({ tri.A.Z, tri.B.Z, tri.C.Z }) } });
        }

        MeshBvh bvh{};
        Blam::MapModel::World::System::BuildBoundsBvh(bounds, bvh.Nodes, bvh.Order);
        return bvh;
    }

    // Whether the parts of an object are exactly the poses armed in [begin, end).
    auto IsSamePoses(const std::vector<ScenePose>& armed, std::size_t begin, std::size_t end, const SceneObject& object) -> bool
    {
        if (end - begin != object.Parts.size()) return false;

        for (std::size_t index{ 0 }; index < object.Parts.size(); ++index)
        {
            const auto& part{ object.Parts[index] };
            if (!(armed[begin + index] == ScenePose{ object.Handle, part.Source, part.Transform })) return false;
        }

        return true;
    }

    // Whether a ray can be cast: its origin, direction and range are numbers. One that is not misses (RV.1).
    auto IsFiniteRay(const Vec3& origin, const Vec3& direction, float maxDistance) -> bool
    {
        return std::isfinite(maxDistance) && Common::Math::System::IsFinite(origin) && Common::Math::System::IsFinite(direction);
    }

    auto StampOf(const RaycastScene& scene) -> SceneStamp
    {
        return SceneStamp{ scene.GathersSinceBuild, scene.FixedBuilds, scene.MobileBuilds, scene.MobileRefits };
    }

    // Whether the reach was gathered from this scene as it is now and for this exclusion: the same handles, whatever their order.
    // note: Equal sizes and every handle of the exclusion in the sorted list of the reach make the two sets equal.
    auto IsReachOf(const SceneReach& reach, const RaycastScene& scene, const SelfExclusion& exclusion) -> bool
    {
        if (!(reach.Stamp == StampOf(scene)) || reach.Excluded.size() != exclusion.size()) return false;

        return std::ranges::all_of(exclusion, [&](std::uint32_t handle) { return std::ranges::binary_search(reach.Excluded, handle); });
    }
}

namespace Views::Raycast::System
{
    auto GatherSceneObjects(ScenePartsCache& reuse, CollidableService& collidableService, const ObjectTableStore& objectStore,
        SceneGatherCounters* counters) -> SceneObjects
    {
        const auto objectTablePtr = objectStore.Acquire();
        if (!objectTablePtr)
        {
            reuse.clear();
            if (counters) *counters = SceneGatherCounters{};
            return SceneObjects{};
        }

        SceneRefs refs{};
        refs.reserve(objectTablePtr->size());
        for (const auto& [handle, object] : *objectTablePtr)
        {
            const bool isFixedPhysics{ object.Placement && (object.Placement->Physics == Physics::Fixed || object.Placement->Physics == Physics::Phased) };
            refs.push_back(SceneRef{ handle, isFixedPhysics, &object });
        }

        const SceneKeyFunction keyOf{ [&](const SceneRef& ref, CollidablePartsKey& key, bool isKnown) { return collidableService.UpdatePartsKey(*ref.Object, key, isKnown); } };
        const ScenePartsFunction partsOf{ [&](std::uint32_t handle) -> std::optional<std::vector<CollidablePart>>
        {
            auto collidable = collidableService.CollectPartsFor(handle);
            if (!collidable) return std::nullopt;

            return std::move(collidable->Parts);
        } };

        return GatherSceneObjects(reuse, refs, keyOf, partsOf, counters);
    }

    auto GatherSceneObjects(ScenePartsCache& reuse, const SceneRefs& refs, const SceneKeyFunction& keyOf, const ScenePartsFunction& partsOf,
        SceneGatherCounters* counters) -> SceneObjects
    {
        SceneObjects objects{};
        objects.reserve(refs.size());

        SceneGatherCounters count{};

        for (const SceneRef& ref : refs)
        {
            const auto [entry, isNew] = reuse.try_emplace(ref.Handle);
            SceneParts& cached{ entry->second };

            const bool isChanged{ keyOf(ref, cached.Key, !isNew) };
            const bool isSame{ !isNew && !isChanged };
            if (isSame)
            {
                ++count.Reused;
                ++cached.StillGathers;
            }
            else
            {
                ++count.Collected;
                auto parts = partsOf(ref.Handle);
                cached.Parts = parts ? std::move(*parts) : std::vector<CollidablePart>{};
                cached.StillGathers = 0;
            }

            cached.IsSeen = true;

            // A fixed or phased object rests while its key does not change, and the rest rest after k_RestGathers equal keys.
            const bool isAtRest{ (ref.IsFixedPhysics && (isSame || isNew)) || cached.StillGathers >= k_RestGathers };
            if (!cached.Parts.empty()) objects.push_back(SceneObject{ ref.Handle, isAtRest, cached.Parts, false });
        }

        // What this gather did not see is gone.
        for (auto entry = reuse.begin(); entry != reuse.end();)
        {
            if (entry->second.IsSeen)
            {
                entry->second.IsSeen = false;
                ++entry;
            }
            else
            {
                entry = reuse.erase(entry);
            }
        }

        if (counters) *counters = count;

        std::ranges::sort(objects, [](const SceneObject& a, const SceneObject& b) { return a.Handle < b.Handle; });
        return objects;
    }

    auto PrepareMeshBvhs(RaycastScene& scene, const SceneObjects& objects, bool covered) -> std::size_t
    {
        std::size_t built{ 0 };
        for (const SceneObject& object : objects)
        {
            if (object.IsCovered != covered) continue;

            for (const auto& part : object.Parts)
            {
                if (!part.Source || part.Source->Triangles.empty() || scene.Meshes.contains(part.Source)) continue;

                scene.Meshes.emplace(part.Source, BuildMeshBvh(*part.Source));
                ++built;
            }
        }

        return built;
    }

    auto ArmFixedScene(RaycastScene& scene, SceneObjects& objects, const FixedRebuildRule& rule) -> bool
    {
        ++scene.GathersSinceBuild;

        const std::vector<ScenePose>& armed{ scene.FixedPoses };
        std::vector<std::uint32_t>& filter{ scene.FixedFilter };
        std::vector<std::uint32_t> left{};
        std::size_t pending{ 0 };
        std::size_t cursor{ 0 };

        const auto groupEnd{ [&](std::size_t begin)
        {
            std::size_t end{ begin };
            while (end < armed.size() && armed[end].Handle == armed[begin].Handle) ++end;
            return end;
        } };
        const auto isFiltered{ [&](std::uint32_t handle) { return std::ranges::binary_search(filter, handle); } };
        const auto leave{ [&](std::uint32_t handle) { if (!isFiltered(handle)) left.push_back(handle); } };

        // Walk the armed objects and the objects at rest together, both by handle.
        for (SceneObject& object : objects)
        {
            object.IsCovered = false;
            if (!object.IsFixed) continue;

            while (cursor < armed.size() && armed[cursor].Handle < object.Handle)
            {
                leave(armed[cursor].Handle);
                cursor = groupEnd(cursor);
            }

            if (cursor == armed.size() || armed[cursor].Handle != object.Handle)
            {
                ++pending;
                continue;
            }

            const std::size_t end{ groupEnd(cursor) };
            if (!isFiltered(object.Handle))
            {
                if (IsSamePoses(armed, cursor, end, object)) object.IsCovered = true;
                else left.push_back(object.Handle);
            }

            cursor = end;
        }

        while (cursor < armed.size())
        {
            leave(armed[cursor].Handle);
            cursor = groupEnd(cursor);
        }

        // What left comes in ascending order and is not in the filter yet.
        const std::ptrdiff_t previous{ static_cast<std::ptrdiff_t>(filter.size()) };
        filter.insert(filter.end(), left.begin(), left.end());
        std::inplace_merge(filter.begin(), filter.begin() + previous, filter.end());

        scene.FixedPending = pending;
        scene.FixedFiltered = filter.size();

        const std::size_t waiting{ pending + filter.size() };
        scene.FixedWaitingGathers = waiting > 0 ? scene.FixedWaitingGathers + 1 : 0;

        const bool isSpaced{ scene.GathersSinceBuild >= rule.Spacing };
        const bool isDue{ scene.FixedBuilds == 0 || (isSpaced && (waiting >= rule.Pending || scene.FixedWaitingGathers >= rule.MaxGathers)) };
        if (waiting == 0 || !isDue) return false;

        std::vector<ScenePose> poses{};
        for (SceneObject& object : objects)
        {
            object.IsCovered = object.IsFixed;
            if (!object.IsFixed) continue;

            for (const auto& part : object.Parts) poses.push_back(ScenePose{ object.Handle, part.Source, part.Transform });
        }

        PrepareMeshBvhs(scene, objects, true);
        ArmTree(scene.Fixed, scene.Meshes, objects, true);
        scene.FixedPoses = std::move(poses);
        filter.clear();
        scene.GathersSinceBuild = 0;
        scene.FixedWaitingGathers = 0;
        ++scene.FixedBuilds;
        return true;
    }

    auto ArmMobileScene(RaycastScene& scene, const SceneObjects& objects, const MobileRebuildRule& rule) -> void
    {
        PrepareMeshBvhs(scene, objects, false);

        SceneTree& tree{ scene.Mobile };
        const bool isSame{ AssemblePieces(tree, scene.Meshes, objects, false) };
        if (isSame && tree.Pieces.empty()) return;

        const std::vector<BvhBounds> bounds{ PieceBounds(tree.Pieces) };
        scene.MobileAreaRatio = 1.0f;
        if (isSame)
        {
            // The boxes of the leaves are the spheres of the parts, so a refit holds exactly what a build would. What can be lost is quality.
            const float area{ Blam::MapModel::World::System::RefitBoundsBvh(bounds, tree.Order, tree.Nodes) };
            if (scene.MobileBuiltArea > 0.0f) scene.MobileAreaRatio = area / scene.MobileBuiltArea;
            if (!(scene.MobileAreaRatio > rule.MaxArea))
            {
                ++scene.MobileRefits;
                return;
            }

            ++scene.MobileDegraded;
        }

        scene.MobileBuiltArea = BuildTree(tree, bounds);
        ++scene.MobileBuilds;
    }

    auto CastScene(const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin, const Vec3& direction,
        float maxDistance, RaycastHit& best, SceneCounters* counters) -> void
    {
        if (!IsFiniteRay(origin, direction, maxDistance)) return;

        CastState state{ best, exclusion, origin, direction, maxDistance, counters, TieKey{} };
        const RayState world{ MakeRayState(origin, direction) };

        CastTree(scene.Fixed, &scene.FixedFilter, world, state);
        CastTree(scene.Mobile, nullptr, world, state);
    }

    auto GatherSceneReach(const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin, float radius, SceneCounters* counters) -> SceneReach
    {
        SceneReach reach{};
        reach.Origin = origin;
        reach.Radius = radius;
        reach.Excluded.assign(exclusion.begin(), exclusion.end());
        std::ranges::sort(reach.Excluded);
        reach.Stamp = StampOf(scene);

        const ExcludedHandles excluded{ exclusion };
        GatherTree(scene.Fixed, &scene.FixedFilter, excluded, reach, counters);
        GatherTree(scene.Mobile, nullptr, excluded, reach, counters);
        return reach;
    }

    auto CastSceneRayReach(const Raycaster& raycaster, const RaycastScene& scene, const SceneReach& reach, const SelfExclusion& exclusion,
        const Vec3& origin, const Vec3& direction, float maxDistance, SceneCounters* counters) -> RaycastHit
    {
        if (!IsFiniteRay(origin, direction, maxDistance)) return CastStatic(raycaster, origin, direction, maxDistance);

        if (origin.X != reach.Origin.X || origin.Y != reach.Origin.Y || origin.Z != reach.Origin.Z || maxDistance > reach.Radius
            || !IsReachOf(reach, scene, exclusion))
        {
            return CastSceneRay(raycaster, scene, exclusion, origin, direction, maxDistance, counters);
        }

        RaycastHit best{ CastStatic(raycaster, origin, direction, maxDistance) };
        CastState state{ best, exclusion, origin, direction, maxDistance, counters, TieKey{} };
        const RayState world{ MakeRayState(origin, direction) };

        // The part of a piece is inside its box, and the ray travels inside the box of its segment, so a piece whose box misses it can not be hit.
        const float reachLimit{ state.Limit() };
        const float endX{ origin.X + direction.X * reachLimit };
        const float endY{ origin.Y + direction.Y * reachLimit };
        const float endZ{ origin.Z + direction.Z * reachLimit };
        const float lowX{ (std::min)(origin.X, endX) };
        const float lowY{ (std::min)(origin.Y, endY) };
        const float lowZ{ (std::min)(origin.Z, endZ) };
        const float highX{ (std::max)(origin.X, endX) };
        const float highY{ (std::max)(origin.Y, endY) };
        const float highZ{ (std::max)(origin.Z, endZ) };

        for (const SceneReachPiece& candidate : reach.Pieces)
        {
            const bool isOverlapping{ static_cast<bool>((candidate.Min[0] <= highX) & (candidate.Max[0] >= lowX) & (candidate.Min[1] <= highY)
                & (candidate.Max[1] >= lowY) & (candidate.Min[2] <= highZ) & (candidate.Max[2] >= lowZ)) };
            if (!isOverlapping) continue;
            if (counters) ++counters->Pieces;

            const ScenePiece& piece{ *candidate.Piece };
            const float limit{ state.Limit() };
            if (!Views::Raycast::System::RayNearPoint(world.Origin, world.Direction, limit, piece.Part.Center, piece.Part.Radius)) continue;
            if (counters) ++counters->NearPieces;

            EnterPiece(piece, world, limit, state);
        }

        return best;
    }

    auto CastSceneRay(const Raycaster& raycaster, const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin,
        const Vec3& direction, float maxDistance, SceneCounters* counters) -> RaycastHit
    {
        RaycastHit best{ CastStatic(raycaster, origin, direction, maxDistance) };
        CastScene(scene, exclusion, origin, direction, maxDistance, best, counters);
        return best;
    }

    auto PrepareScene(RaycastScene& scene, ScenePartsCache& parts, CollidableService& collidableService, const ObjectTableStore& objectStore) -> void
    {
        SceneObjects objects{ GatherSceneObjects(parts, collidableService, objectStore, nullptr) };
        ArmFixedScene(scene, objects);
        ArmMobileScene(scene, objects);

        ++scene.Prepares;
    }
}