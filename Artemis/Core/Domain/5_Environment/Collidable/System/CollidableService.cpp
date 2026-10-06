module Environment.Collidable.System;

import Tables.Object.Type;

namespace
{
    using Triangle = Common::Math::Type::Triangle;
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

            instances.push_back(this->BuildInstance(object, objectTable));
        }

        m_CollidableStore.Publish(std::move(instances));
    }

    auto CollidableService::BuildInstance(const AliveObject& object,
        const ObjectTable& objectTable) -> Collidable
    {
        Collidable instance{};
        instance.Handle = object.Handle;
        instance.TagName = object.TagName;
        instance.Position = object.Position;
        instance.Forward = object.Forward;
        instance.Up = object.Up;

        Context ctx{};
        ctx.Coll = m_WorldStore.GetResolvedCollForObject(object.TagName, m_DefinitionsStore);

        const BoneMatrixTable* bones = m_BoneMatricesStore.Get(object.Handle);
        const DamageSectionTable* damage = m_DamageSectionsStore.Get(object.Handle);

        instance.AncestorDead = this->IsAncestorDead(object.Handle, objectTable);

        if (ctx.Coll)
        {
            const ResolvedRegionStates* states =
                m_WorldStore.GetResolvedRegionStates(object.TagName);

            if (states && object.HlmtVariant < states->Variants.size())
            {
                ctx.States = states;
                ctx.Variant = object.HlmtVariant;
                instance.HasDestroyedGeometry =
                    states->Variants[object.HlmtVariant].HasDestroyedGeometry;
            }
        }

        instance.WorldMesh = this->CollectMesh(instance, ctx, bones, damage);
        instance.Parts = this->CollectParts(instance, ctx, bones, damage);

        return instance;
    }

    auto CollidableService::CollectMeshFor(std::uint32_t handle) -> std::optional<Collidable>
    {
        auto objectTablePtr = m_ObjectTableStore.Acquire();
        if (!objectTablePtr) return std::nullopt;

        auto it = objectTablePtr->find(handle);
        if (it == objectTablePtr->end() || it->second.Address == 0) return std::nullopt;

        return this->BuildInstance(it->second, *objectTablePtr);
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
            return this->CollectSkeletal(instance, ctx, *bones, damage);
        }

        return this->CollectRigid(instance, ctx, damage);
    }

    auto CollidableService::CollectSkeletal(const Collidable& instance,
        const Context& ctx, const BoneMatrixTable& bones, const DamageSectionTable* damage) -> CollMesh
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
            if (!this->IsActivePermutation(instance, ctx, mesh, damage))
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
            if (!this->IsActivePermutation(instance, ctx, mesh, damage)) continue;

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
                if (!this->IsActivePermutation(instance, ctx, mesh, damage)) continue;

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
            if (!this->IsActivePermutation(instance, ctx, mesh, damage)) continue;

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

    auto CollidableService::IsActivePermutation(const Collidable& instance,
        const Context& ctx, const CollMesh& mesh,
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

        const ResolvedRegionStates* states = ctx.States;
        const StateMap* stateMap = states ?
            &states->Variants[ctx.Variant].StateMap : nullptr;

        if (!damage || damage->Sections.empty() ||
            !stateMap || stateMap->empty() || mesh.RegionIndex < 0 ||
            (size_t)mesh.RegionIndex >= stateMap->size() ||
            (size_t)mesh.RegionIndex >= states->RegionToSection.size())
        {
            return isDefault(mesh.RegionIndex);
        }

        const auto& sections = damage->Sections;
        const auto& row = (*stateMap)[mesh.RegionIndex];
        const int slot = states->RegionToSection[mesh.RegionIndex];

        const bool objectDead = (!sections.empty() &&
            sections[0].Vitality <= 0.0f) || instance.AncestorDead;

        if (objectDead && instance.HasDestroyedGeometry)
        {
            if (row[4] >= 0)
            {
                return mesh.PermutationIndex == row[4];
            }

            return false;
        }

        int deathState = -1;
        if ((size_t)mesh.RegionIndex < states->DeathStateMap.size())
        {
            deathState = states->DeathStateMap[mesh.RegionIndex];
        }

        int highestMapped = -1;
        for (int section = 4; section >= 0; --section)
        {
            if (row[section] >= 0)
            {
                highestMapped = section;
                break;
            }
        }

        int defaultPermutation = 0;
        if (ctx.Coll && mesh.RegionIndex >= 0 &&
            (size_t)mesh.RegionIndex <
            ctx.Coll->DefaultPermutationIndex.size())
        {
            const int def =
                ctx.Coll->DefaultPermutationIndex[mesh.RegionIndex];

            defaultPermutation = (def > 0) ? def : 0;
        }

        if (slot < 0 || (size_t)slot >= sections.size())
        {
            if (objectDead && deathState >= 0)
            {
                if (deathState <= highestMapped)
                {
                    return mesh.PermutationIndex == row[deathState];
                }

                int section = highestMapped;
                int permutation = (section >= 0) ? row[section] : -1;
                while (permutation < 0 && section > 0)
                {
                    --section;
                    permutation = row[section];
                }

                if (permutation < 0 || permutation == defaultPermutation)
                {
                    return false;
                }

                return mesh.PermutationIndex == permutation;
            }

            return isDefault(mesh.RegionIndex);
        }

        bool hasAnyMapping = false;
        for (int section = 0; section < 5; ++section)
        {
            if (row[section] >= 0)
            {
                hasAnyMapping = true;
                break;
            }
        }

        if (!hasAnyMapping)
        {
            if (objectDead && deathState >= 0)
            {
                return false;
            }

            return isDefault(mesh.RegionIndex);
        }

        int state = 0;
        const uint16_t mask = sections[slot].DamageLevelMask;
        const int level = this->HighestLevelFromMask(mask);

        if (level >= 0 &&
            (size_t)mesh.RegionIndex < states->LevelToState.size())
        {
            const auto& levels = states->LevelToState[mesh.RegionIndex];

            for (int current = (std::min)(level, (int)levels.size() - 1);
                current >= 0;
                --current)
            {
                if (levels[current] >= 0)
                {
                    state = levels[current];
                    break;
                }
            }
        }

        if (objectDead && deathState >= 0)
        {
            state = (std::max)(state, deathState);

            if (state > highestMapped)
            {
                int section = highestMapped;
                int permutation = (section >= 0) ? row[section] : -1;
                while (permutation < 0 && section > 0)
                {
                    --section;
                    permutation = row[section];
                }

                if (permutation < 0 || permutation == defaultPermutation)
                {
                    return false;
                }

                state = section;
            }
        }
        else
        {
            if (state > highestMapped)
            {
                state = highestMapped;
            }
        }

        int wantPermutation = row[state];
        while (wantPermutation < 0 && state > 0)
        {
            --state;
            wantPermutation = row[state];
        }

        if (wantPermutation < 0)
        {
            return isDefault(mesh.RegionIndex);
        }

        return mesh.PermutationIndex == wantPermutation;
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

    auto CollidableService::HighestLevelFromMask(std::uint16_t mask) -> int
    {
        if (mask == 0) return -1;

        int level = -1;
        for (int b = 0; b < 16; ++b)
        {
            if (mask & (1u << b))
            {
                level = b;
            }
        }

        return level;
    }

    auto CollidableService::IsAncestorDead(std::uint32_t handle,
        const ObjectTable& objects) -> bool
    {
        uint32_t current = m_ObjectGraphStore.GetParent(handle);
        int guard = 0;

        while (current != 0 && current != 0xFFFFFFFF && guard++ < 16)
        {
            auto it = objects.find(current);
            if (it != objects.end() && it->second.Address != 0)
            {
                const DamageSectionTable* dmg = m_DamageSectionsStore.Get(current);

                if (dmg && !dmg->Sections.empty() &&
                    dmg->Sections[0].Vitality <= 0.0f)
                {
                    return true;
                }
            }

            const uint32_t parent = m_ObjectGraphStore.GetParent(current);
            if (parent == current) break;
            current = parent;
        }

        return false;
    }

    auto CollidableService::Cleanup() -> void
    {
        m_CollidableStore.Cleanup();
        m_LogsService.Message("[CollidableService] INFO: Cleanup completed.");
    }
}
