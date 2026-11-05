module Perception.Collidable.System;

import Common.Math.System;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Collidable.Type;

namespace
{
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Coll::Coll;
    using Perception::Collidable::Type::Collidable;
    using Perception::Collidable::Type::RegionDiagnostic;

    using Perception::Collidable::Type::k_RegionStateStride;
    using Perception::Collidable::Type::k_RegionStateValue;
    using Perception::Collidable::Type::k_NoPermutation;
    using Perception::Collidable::Type::Collidables;

    // Axes of a rigid object (no bones) and the center of its collision bounds, in the object space.
    struct RigidFrame
    {
        Vec3 Position{};
        Vec3 Forward{};
        Vec3 Up{};
        Vec3 Right{};
        Vec3 Center{};
    };

    auto MakeRigidFrame(const Collidable& instance, const Coll& coll) -> RigidFrame
    {
        return RigidFrame{
            .Position = instance.Position,
            .Forward = instance.Forward,
            .Up = instance.Up,
            .Right = Common::Math::System::Cross(instance.Up, instance.Forward),
            .Center = Vec3{ (coll.BoundsMin.X + coll.BoundsMax.X) * 0.5f,
                            (coll.BoundsMin.Y + coll.BoundsMax.Y) * 0.5f,
                            (coll.BoundsMin.Z + coll.BoundsMax.Z) * 0.5f }
        };
    }

    // Appends the triangles with their points placed by pose, keeping surface flags and material.
    auto AppendPosed(std::vector<Triangle>& out, const auto& triangles, const auto& pose) -> void
    {
        for (const auto& triangle : triangles)
        {
            Triangle worldTriangle;
            worldTriangle.A = pose(triangle.A);
            worldTriangle.B = pose(triangle.B);
            worldTriangle.C = pose(triangle.C);
            worldTriangle.SurfaceFlags = triangle.SurfaceFlags;
            worldTriangle.Material = triangle.Material;
            out.push_back(worldTriangle);
        }
    }
}

namespace Perception::Collidable::System
{
    auto CollidableService::BuildScene() -> std::optional<Collidables>
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectTableStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return std::nullopt;

        Collidables instances{};
        instances.reserve(objectTablePtr->size());

        for (const auto& [handle, object] : *objectTablePtr)
        {
            instances.push_back(this->BuildInstance(object, true));
        }

        return instances;
    }

    auto CollidableService::BuildInstance(const AliveObject& object, bool buildWorldMesh) -> Collidable
    {
        Collidable instance{};
        instance.Handle = object.Handle;
        instance.TagName = object.TagName;
        instance.Position = object.BoundingCenter;
        instance.Forward = object.Forward;
        instance.Up = object.Up;

        Context ctx{};
        ctx.Coll = m_WorldStore.GetCollForObject(object.TagName, m_DefinitionsStore);

        const BoneMatrixTable* bones = m_BoneMatricesStore.Get(object.Handle);

        // Objects without a render model (like the invisible spawn points) reserve the bone block but never
        // write it, so their matrices are not valid. They are placed like a rigid object instead.
        if (bones != nullptr && !std::ranges::any_of(bones->Matrices, Blam::Runtime::Object::Type::BoneMatrix::IsBoneMatrixValid))
        {
            bones = nullptr;
        }

        const DamageSectionTable* damage = m_DamageSectionsStore.Get(object.Handle);

        if (ctx.Coll)
        {
            ctx.States = m_WorldStore.GetRegionStates(object.TagName);
        }

        if (buildWorldMesh)
        {
            instance.WorldMesh = this->CollectMesh(instance, ctx, bones, damage);
        }

        instance.Parts = this->CollectParts(instance, ctx, bones, damage);

        this->BuildRegionDiagnostics(instance, ctx, damage);

        return instance;
    }

    auto CollidableService::CollectPartsFor(std::uint32_t handle) -> std::optional<Collidable>
    {
        auto objectTablePtr = m_ObjectTableStore.Acquire();
        if (!objectTablePtr) return std::nullopt;

        auto it = objectTablePtr->find(handle);
        if (it == objectTablePtr->end()) return std::nullopt;

        return this->BuildInstance(it->second, false);
    }

    auto CollidableService::QueryNearby(const Vec3& origin, float radius) const -> std::vector<std::uint32_t>
    {
        std::vector<std::uint32_t> out;

        auto objectTablePtr = m_ObjectTableStore.Acquire();
        if (!objectTablePtr) return out;

        const float radiusSq = radius * radius;

        for (const auto& [handle, object] : *objectTablePtr)
        {
            const float dx = object.BoundingCenter.X - origin.X;
            const float dy = object.BoundingCenter.Y - origin.Y;
            const float dz = object.BoundingCenter.Z - origin.Z;

            if (dx * dx + dy * dy + dz * dz <= radiusSq)
            {
                out.push_back(handle);
            }
        }

        return out;
    }

    auto CollidableService::CollectMesh(const Collidable& instance,
        const Context& ctx, const BoneMatrixTable* bones, const DamageSectionTable* damage) -> CollMesh
    {
        if (!ctx.Coll) return {};

        if (bones != nullptr)
        {
            return this->CollectSkeletal(ctx, *bones, damage);
        }

        return this->CollectRigid(instance, ctx, damage);
    }

    auto CollidableService::CollectSkeletal(const Context& ctx,
        const BoneMatrixTable& bones, const DamageSectionTable* damage) -> CollMesh
    {
        CollMesh out;

        if (bones.Matrices.empty() || !ctx.Coll)
        {
            return out;
        }

        const auto& matrices = bones.Matrices;

        size_t totalTriangles = 0;
        for (const auto& mesh : ctx.Coll->Meshes)
        {
            totalTriangles += mesh.Triangles.size();
        }
        out.Triangles.reserve(totalTriangles);

        for (const auto& mesh : ctx.Coll->Meshes)
        {
            if (!this->IsActivePermutation(ctx, mesh, damage))
            {
                continue;
            }

            const BoneMatrix* bone = Blam::Runtime::Object::Type::BoneMatrix::FindValidBone(matrices, mesh.NodeIndex);
            if (bone == nullptr)
            {
                continue;
            }

            AppendPosed(out.Triangles, mesh.Triangles, [&](const Vec3& v)
            {
                return Blam::Runtime::Object::Type::BoneMatrix::Transform(*bone, v.X, v.Y, v.Z);
            });
        }

        return out;
    }

    auto CollidableService::CollectRigid(const Collidable& instance,
        const Context& ctx, const DamageSectionTable* damage) -> CollMesh
    {
        CollMesh out;
        if (!ctx.Coll) return out;

        const RigidFrame frame{ MakeRigidFrame(instance, *ctx.Coll) };

        auto poseLocal = [&](const Vec3& v)
        {
            return this->TransformPoint(frame.Position, frame.Right, frame.Forward, frame.Up,
                v.X - frame.Center.X, v.Y - frame.Center.Y, v.Z - frame.Center.Z);
        };

        size_t triTotal = 0;
        for (const auto& mesh : ctx.Coll->Meshes)
        {
            triTotal += mesh.Triangles.size();
        }
        out.Triangles.reserve(triTotal);

        for (const auto& mesh : ctx.Coll->Meshes)
        {
            if (!this->IsActivePermutation(ctx, mesh, damage)) continue;

            AppendPosed(out.Triangles, mesh.Triangles, poseLocal);
        }

        return out;
    }

    auto CollidableService::CollectParts(const Collidable& instance,
        const Context& ctx, const BoneMatrixTable* bones,
        const DamageSectionTable* damage) -> std::vector<CollidablePart>
    {
        std::vector<CollidablePart> out;
        if (!ctx.Coll) return out;

        const auto& meshes = ctx.Coll->Meshes;
        out.reserve(meshes.size());

        if (bones != nullptr)
        {
            const auto& matrices = bones->Matrices;
            if (matrices.empty()) return out;

            for (const auto& mesh : meshes)
            {
                if (!this->IsActivePermutation(ctx, mesh, damage)) continue;

                const BoneMatrix* bone = Blam::Runtime::Object::Type::BoneMatrix::FindValidBone(matrices, mesh.NodeIndex);
                if (bone == nullptr)
                {
                    continue;
                }

                const auto& r = bone->Rotation;
                const auto& t = bone->Translation;

                CollidablePart part;
                part.Source = &mesh;
                part.Transform = {
                    r[0], r[3], r[6], t[0],
                    r[1], r[4], r[7], t[1],
                    r[2], r[5], r[8], t[2] };

                out.push_back(part);
            }

            return out;
        }

        const RigidFrame frame{ MakeRigidFrame(instance, *ctx.Coll) };
        const auto& fwd = frame.Forward;
        const auto& up = frame.Up;
        const auto& rgt = frame.Right;
        const float cx = frame.Center.X;
        const float cy = frame.Center.Y;
        const float cz = frame.Center.Z;

        const float tx = frame.Position.X - (fwd.X * cx + rgt.X * cy + up.X * cz);
        const float ty = frame.Position.Y - (fwd.Y * cx + rgt.Y * cy + up.Y * cz);
        const float tz = frame.Position.Z - (fwd.Z * cx + rgt.Z * cy + up.Z * cz);

        for (const auto& mesh : meshes)
        {
            if (!this->IsActivePermutation(ctx, mesh, damage)) continue;

            CollidablePart part;
            part.Source = &mesh;
            part.Transform = {
                fwd.X, rgt.X, up.X, tx,
                fwd.Y, rgt.Y, up.Y, ty,
                fwd.Z, rgt.Z, up.Z, tz };

            out.push_back(part);
        }

        return out;
    }

    auto CollidableService::IsActivePermutation(const Context& ctx, const CollMesh& mesh,
        const DamageSectionTable* damage) -> bool
    {
        auto isDefault = [&](int regionIdx) -> bool {
            if (!ctx.Coll || regionIdx < 0 ||
                (size_t)regionIdx >=
                ctx.Coll->DefaultPermutationIndex.size())
            {
                return mesh.PermutationIndex == 0;
            }

            const int def = ctx.Coll->DefaultPermutationIndex[regionIdx];

            return def < 0 ? (mesh.PermutationIndex == 0) :
                (mesh.PermutationIndex == def);
            };

        if (!damage || !ctx.States || mesh.RegionIndex < 0)
        {
            return isDefault(mesh.RegionIndex);
        }

        const auto& bytes = damage->Regions.Bytes;
        const auto& engineRegions = ctx.States->EngineRegions;
        const std::size_t regionCount = engineRegions.size();
        const std::size_t permutationsBase = regionCount * k_RegionStateStride;

        if (regionCount == 0 || bytes.size() < permutationsBase + regionCount)
        {
            return isDefault(mesh.RegionIndex);
        }

        for (std::size_t region = 0; region < regionCount; ++region)
        {
            if (engineRegions[region].CollRegion != mesh.RegionIndex) continue;

            const std::uint8_t engine = bytes[permutationsBase + region];

            if (engine == k_NoPermutation) return false;

            if (engine < engineRegions[region].CollPermutations.size())
            {
                const int mapped = engineRegions[region].CollPermutations[engine];

                if (mapped >= 0) return mesh.PermutationIndex == mapped;
            }

            // The collision model has no permutation with the name of the one the engine shows.
            // The region still exists when the render model draws it, so it keeps its default.
            const auto& meshCounts = engineRegions[region].PermutationMeshCounts;

            if (engine < meshCounts.size() && meshCounts[engine] <= 0) return false;

            return isDefault(mesh.RegionIndex);
        }

        return isDefault(mesh.RegionIndex);
    }

    auto CollidableService::BuildRegionDiagnostics(Collidable& instance, const Context& ctx,
        const DamageSectionTable* damageSectionTable) -> void
    {
        if (!ctx.Coll || !ctx.States || !damageSectionTable ||
            damageSectionTable->Regions.Bytes.empty()) return;

        const auto& engineRegions = ctx.States->EngineRegions;
        const auto& bytes = damageSectionTable->Regions.Bytes;

        instance.RegionBlockSize = damageSectionTable->Regions.Size;
        instance.RegionBlockOffset = damageSectionTable->Regions.Offset;
        instance.RegionBlock = bytes;

        // note: The block is, per region of the render model (n): n entries of 4 bytes with the state at +1,
        // then n permutation bytes, then n bytes that are still unknown.
        const std::size_t regionCount = engineRegions.size();
        const std::size_t permutationsBase = regionCount * k_RegionStateStride;

        instance.Regions.resize(regionCount);

        for (std::size_t region = 0; region < regionCount; ++region)
        {
            RegionDiagnostic& entry = instance.Regions[region];
            const auto& engineRegion = engineRegions[region];

            entry.Name = engineRegion.Name;
            entry.CollRegion = engineRegion.CollRegion;

            const std::size_t stateIndex = region * k_RegionStateStride + k_RegionStateValue;
            const std::size_t permutationIndex = permutationsBase + region;

            if (stateIndex < bytes.size())
            {
                entry.EngineState = static_cast<int>(bytes[stateIndex]);
            }

            if (permutationIndex < bytes.size() && bytes[permutationIndex] != k_NoPermutation)
            {
                entry.EnginePermutation = static_cast<int>(bytes[permutationIndex]);

                if (static_cast<std::size_t>(entry.EnginePermutation) < engineRegion.CollPermutations.size())
                {
                    entry.MappedPermutation = engineRegion.CollPermutations[entry.EnginePermutation];
                }

                if (static_cast<std::size_t>(entry.EnginePermutation) < engineRegion.PermutationMeshCounts.size())
                {
                    entry.EngineMeshCount = engineRegion.PermutationMeshCounts[entry.EnginePermutation];
                }
            }

            if (entry.CollRegion < 0) continue;

            for (const CollidablePart& part : instance.Parts)
            {
                if (part.Source && part.Source->RegionIndex == entry.CollRegion)
                {
                    entry.ShownPermutations.push_back(static_cast<int>(part.Source->PermutationIndex));
                }
            }
        }
    }

    // --- Helpers ---

    auto CollidableService::TransformPoint(const Vec3& pos, const Vec3& right,
        const Vec3& forward, const Vec3& up, float lx, float ly, float lz) -> Vec3
    {
        return {
            pos.X + forward.X * lx + right.X * ly + up.X * lz,
            pos.Y + forward.Y * lx + right.Y * ly + up.Y * lz,
            pos.Z + forward.Z * lx + right.Z * ly + up.Z * lz
        };
    }

    auto CollidableService::Cleanup() -> void
    {
        m_Log.Message("[CollidableService] INFO: Cleanup completed.");
    }
}