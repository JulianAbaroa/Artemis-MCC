module;

#include <cmath>

module Views.Raycast.System;

import Common.Math.Type;
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
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::FixedRebuildRule;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::MeshBvh;
    using Views::Raycast::Type::MeshBvhs;
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
        const bool hitLeft = Blam::MapModel::World::System::RayEntersNode(nodes[static_cast<std::size_t>(left)], ray.Position, ray.Inverse, limit, leftEnter);
        const bool hitRight = Blam::MapModel::World::System::RayEntersNode(nodes[static_cast<std::size_t>(left) + 1], ray.Position, ray.Inverse, limit, rightEnter);

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

        std::array<std::int32_t, k_StackSize> stack{};
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

    // Goes down one instance tree and into the mesh of every piece the ray reaches.
    // param filtered: Sorted handles whose pieces are skipped. Null for none.
    auto CastTree(const SceneTree& tree, const std::vector<std::uint32_t>* filtered, const RayState& world, CastState& state) -> void
    {
        if (tree.Nodes.empty()) return;

        std::array<std::int32_t, k_StackSize> stack{};
        int top{ 0 };
        stack[top++] = 0;

        while (top > 0)
        {
            const BvhNode& node = tree.Nodes[static_cast<std::size_t>(stack[--top])];
            if (state.Counters) ++state.Counters->Nodes;

            if (node.Count > 0)
            {
                for (std::int32_t slot = node.Left; slot < node.Left + node.Count; ++slot)
                {
                    const ScenePiece& piece{ tree.Pieces[tree.Order[static_cast<std::size_t>(slot)]] };
                    if (state.Exclusion.contains(piece.Handle)) continue;
                    if (filtered && std::ranges::binary_search(*filtered, piece.Handle)) continue;

                    const float limit{ state.Limit() };
                    if (!Views::Raycast::System::RayNearPoint(world.Origin, world.Direction, limit, piece.Part.Center, piece.Part.Radius)) continue;

                    RayState local{};
                    Views::Raycast::System::PoseRay(piece.Part.ToLocal, world.Origin, world.Direction, local.Origin, local.Direction);
                    if (!Views::Raycast::System::RayHitsBox(local.Origin, local.Direction, piece.Part.Source->LocalMin, piece.Part.Source->LocalMax, limit)) continue;

                    local = MakeRayState(local.Origin, local.Direction);
                    CastMesh(piece, local, state);
                }

                continue;
            }

            PushChildren(tree.Nodes, node.Left, world, state.Limit(), stack, top);
        }
    }

    // Builds an instance tree over the parts of the objects whose IsCovered is covered.
    auto ArmTree(SceneTree& tree, const MeshBvhs& meshes, const SceneObjects& objects, bool covered) -> void
    {
        tree.Pieces.clear();

        for (const SceneObject& object : objects)
        {
            if (object.IsCovered != covered) continue;

            const std::vector<CachedPart> parts{ Views::Raycast::System::PrepareParts(object.Parts) };
            for (std::size_t index{ 0 }; index < parts.size(); ++index)
            {
                ScenePiece piece{};
                piece.Part = parts[index];
                piece.Bvh = &meshes.at(parts[index].Source);
                piece.Handle = object.Handle;
                piece.PartIndex = static_cast<std::uint32_t>(index);
                tree.Pieces.push_back(piece);
            }
        }

        // The box of the sphere of the part: it holds the part whatever its pose.
        std::vector<BvhBounds> bounds{};
        bounds.reserve(tree.Pieces.size());
        for (const ScenePiece& piece : tree.Pieces)
        {
            const Vec3& c{ piece.Part.Center };
            const float r{ piece.Part.Radius + k_BoxPadding };
            bounds.push_back(BvhBounds{ { c.X - r, c.Y - r, c.Z - r }, { c.X + r, c.Y + r, c.Z + r } });
        }

        Blam::MapModel::World::System::BuildBoundsBvh(bounds, tree.Nodes, tree.Order);
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

    auto ArmMobileScene(RaycastScene& scene, const SceneObjects& objects) -> void
    {
        PrepareMeshBvhs(scene, objects, false);
        ArmTree(scene.Mobile, scene.Meshes, objects, false);
    }

    auto CastScene(const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin, const Vec3& direction,
        float maxDistance, RaycastHit& best, SceneCounters* counters) -> void
    {
        CastState state{ best, exclusion, origin, direction, maxDistance, counters, TieKey{} };
        const RayState world{ MakeRayState(origin, direction) };

        CastTree(scene.Fixed, &scene.FixedFilter, world, state);
        CastTree(scene.Mobile, nullptr, world, state);
    }

    auto CastSceneRay(const Raycaster& raycaster, const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin,
        const Vec3& direction, float maxDistance, SceneCounters* counters) -> RaycastHit
    {
        RaycastHit best{ CastStatic(raycaster, origin, direction, maxDistance) };
        CastScene(scene, exclusion, origin, direction, maxDistance, best, counters);
        return best;
    }
}