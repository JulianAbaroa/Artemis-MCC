module Environment.Collidable.System;

import Tables.Object.Type;

namespace
{
    using Triangle = Common::Math::Type::Triangle;
    using RegionDiagnostic = Environment::Collidable::Type::RegionDiagnostic;

    // Layout of the region block: one state entry per region of the render model, then one permutation byte per region.
    constexpr std::size_t k_RegionStateStride{ 0x04 };
    constexpr std::size_t k_RegionStateValue{ 0x01 };
    constexpr std::uint8_t k_NoPermutation{ 0xFF };
}

namespace Environment::Collidable::System
{
    auto CollidableService::Update(bool isDebugViewActive) -> void
    {
        if (!isDebugViewActive)
        {
            m_CollidableStore.Publish({});
            return;
        }

        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectTableStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        this->CollectCollidables(*classifiedsPtr, *objectTablePtr);
    }

    auto CollidableService::CollectCollidables(const Classifieds& classifieds,
        const ObjectTable& objectTable) -> void
    {
        std::vector<Collidable> instances;
        instances.reserve(objectTable.size());

        for (const auto& [handle, object] : objectTable)
        {
            if (object.Address == 0) continue;

            instances.push_back(this->BuildInstance(object, true));
        }

        m_CollidableStore.Publish(std::move(instances));
    }

    auto CollidableService::BuildInstance(const AliveObject& object, bool buildWorldMesh) -> Collidable
    {
        Collidable instance{};
        instance.Handle = object.Handle;
        instance.TagName = object.TagName;
        instance.Position = object.Position;
        instance.Forward = object.Forward;
        instance.Up = object.Up;

        Context ctx{};
        ctx.Coll = m_WorldStore.GetCollForObject(object.TagName, m_DefinitionsStore);

        const BoneMatrixTable* bones = m_BoneMatricesStore.Get(object.Handle);
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
        if (it == objectTablePtr->end() || it->second.Address == 0) return std::nullopt;

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
            if (object.Address == 0) continue;

            const float dx = object.Position.X - origin.X;
            const float dy = object.Position.Y - origin.Y;
            const float dz = object.Position.Z - origin.Z;

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

            if (mesh.NodeIndex < 0 || static_cast<size_t>(mesh.NodeIndex) >= matrices.size())
            {
                continue;
            }

            const BoneMatrix& bone = matrices[mesh.NodeIndex];

            if (!Tables::Object::Type::BoneMatrix::IsBoneMatrixValid(bone))
            {
                continue;
            }

            auto poseLocal = [&](const Vec3& v) {
                const auto& r = bone.Rotation;
                return Vec3{
                    r[0] * v.X + r[3] * v.Y + r[6] * v.Z + bone.Translation[0],
                    r[1] * v.X + r[4] * v.Y + r[7] * v.Z + bone.Translation[1],
                    r[2] * v.X + r[5] * v.Y + r[8] * v.Z + bone.Translation[2]
                };
                };

            for (const auto& triangle : mesh.Triangles)
            {
                Triangle worldTriangle;
                worldTriangle.A = poseLocal(triangle.A);
                worldTriangle.B = poseLocal(triangle.B);
                worldTriangle.C = poseLocal(triangle.C);
                worldTriangle.SurfaceFlags = triangle.SurfaceFlags;
                worldTriangle.Material = triangle.Material;
                out.Triangles.push_back(worldTriangle);
            }
        }

        return out;
    }

    auto CollidableService::CollectRigid(const Collidable& instance,
        const Context& ctx, const DamageSectionTable* damage) -> CollMesh
    {
        CollMesh out;
        if (!ctx.Coll) return out;

        const auto& pos = instance.Position;
        const auto& fwd = instance.Forward;
        const auto& up = instance.Up;
        const auto  rgt = this->Cross(up, fwd);

        const float cx = (ctx.Coll->BoundsMin.X + ctx.Coll->BoundsMax.X) * 0.5f;
        const float cy = (ctx.Coll->BoundsMin.Y + ctx.Coll->BoundsMax.Y) * 0.5f;
        const float cz = (ctx.Coll->BoundsMin.Z + ctx.Coll->BoundsMax.Z) * 0.5f;

        auto poseLocal = [&](const Vec3& v) {
            return this->TransformPoint(pos, rgt, fwd, up,
                v.X - cx, v.Y - cy, v.Z - cz);
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

            for (const auto& triangle : mesh.Triangles)
            {
                Triangle worldTriangle;
                worldTriangle.A = poseLocal(triangle.A);
                worldTriangle.B = poseLocal(triangle.B);
                worldTriangle.C = poseLocal(triangle.C);
                worldTriangle.SurfaceFlags = triangle.SurfaceFlags;
                worldTriangle.Material = triangle.Material;
                out.Triangles.push_back(worldTriangle);
            }
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

                if (mesh.NodeIndex < 0 || static_cast<size_t>(mesh.NodeIndex) >= matrices.size())
                {
                    continue;
                }

                const BoneMatrix& bone = matrices[mesh.NodeIndex];

                if (!Tables::Object::Type::BoneMatrix::IsBoneMatrixValid(bone))
                {
                    continue;
                }

                const auto& r = bone.Rotation;
                const auto& t = bone.Translation;

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

        const auto& pos = instance.Position;
        const auto& fwd = instance.Forward;
        const auto& up = instance.Up;
        const auto  rgt = this->Cross(up, fwd);

        const float cx = (ctx.Coll->BoundsMin.X + ctx.Coll->BoundsMax.X) * 0.5f;
        const float cy = (ctx.Coll->BoundsMin.Y + ctx.Coll->BoundsMax.Y) * 0.5f;
        const float cz = (ctx.Coll->BoundsMin.Z + ctx.Coll->BoundsMax.Z) * 0.5f;

        const float tx = pos.X - (fwd.X * cx + rgt.X * cy + up.X * cz);
        const float ty = pos.Y - (fwd.Y * cx + rgt.Y * cy + up.Y * cz);
        const float tz = pos.Z - (fwd.Z * cx + rgt.Z * cy + up.Z * cz);

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

    auto CollidableService::Cross(const Vec3& a, const Vec3& b) -> Vec3
    {
        return {
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X
        };
    }

    auto CollidableService::TransformPoint(const Vec3& pos, const Vec3& right,
        const Vec3& forward, const Vec3& up, float lx, float ly, float lz) -> Vec3
    {
        return {
            pos.X + forward.X * lx + right.X * ly + up.X * lz,
            pos.Y + forward.Y * lx + right.Y * ly + up.Y * lz,
            pos.Z + forward.Z * lx + right.Z * ly + up.Z * lz
        };
    }

    auto CollidableService::TransformByBone(const BoneMatrix& m,
        float lx, float ly, float lz) -> Vec3
    {
        const auto& r = m.Rotation;
        const float wx = r[0] * lx + r[1] * ly + r[2] * lz + m.Translation[0];
        const float wy = r[3] * lx + r[4] * ly + r[5] * lz + m.Translation[1];
        const float wz = r[6] * lx + r[7] * ly + r[8] * lz + m.Translation[2];
        return { wx, wy, wz };
    }

    auto CollidableService::Cleanup() -> void
    {
        m_CollidableStore.Cleanup();
        m_LogsService.Message("[CollidableService] INFO: Cleanup completed.");
    }
}
