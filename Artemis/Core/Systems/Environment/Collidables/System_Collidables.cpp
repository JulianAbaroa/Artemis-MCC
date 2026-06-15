#include "pch.h"

#include "System_Collidables.h"

#include "Core/Types/Sources/Tables/Object/LiveObject.h"
#include "Core/Types/Structure/Classified/Classified.h"

#include "Core/States/Sources/MapReader/Tags/State_MapHlmt.h"
#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Sources/Tables/Object/BoneMatrix/State_BoneMatrices.h"
#include "Core/States/Sources/Tables/Object/DamageSection/State_DamageSections.h"
#include "Core/States/Structure/Classifier/State_Classifier.h"
#include "Core/States/Structure/Graph/Object/State_ObjectGraph.h"
#include "Core/States/Sources/Static/World/State_WorldBuilder.h"
#include "Core/States/Environment/Collidables/State_Collidables.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include "Generated/Coll/CollObject.h"

#include <unordered_set>

void System_Collidables::Update()
{
    auto classifiedsPtr = m_Deps.State_Classifier.Acquire();
    auto objectTablePtr = m_Deps.State_ObjectTable.Acquire();
    if (!classifiedsPtr || ! objectTablePtr) return;

    this->CollectCollidables(*classifiedsPtr, *objectTablePtr);
}

void System_Collidables::CollectCollidables(
    const Classifieds& classifieds, 
    const ObjectTable& objectTable)
{
    std::vector<Collidable> instances;

    for (const auto& [handle, object] : objectTable)
    {
        if (object.Address == 0) continue;

        Collidable instance{};
        instance.Handle = object.Handle;
        instance.TagName = object.TagName;
        instance.Position = object.Position;
        instance.Forward = object.Forward;
        instance.Up = object.Up;

        const std::string collName = m_Deps.State_WorldBuilder.
            ResolveObjectCollName(object.TagName);
        instance.Coll = m_Deps.State_WorldBuilder.GetCollGeometry(collName);

        const BoneMatrixTable* bones = 
            m_Deps.State_BoneMatrices.Get(object.Handle);

        const DamageSectionTable* damage = 
            m_Deps.State_DamageSections.Get(object.Handle);

        instance.AncestorDead =
            this->IsAncestorDead(object.Handle, objectTable);

        if (instance.Coll)
        {
            const std::string hlmtName = m_Deps.State_WorldBuilder.
                ResolveObjectHlmtName(object.TagName);
            const HlmtObject* hlmt = m_Deps.State_MapHlmt.Get(hlmtName);

            if (hlmt && object.HlmtVariant < hlmt->Variants.size())
            {
                instance.StateMap = this->CollectStateMap(
                    hlmt->Variants[object.HlmtVariant], 
                    *instance.Coll);

                instance.HasDestroyedGeometry = false;
                for (const auto& region : instance.StateMap)
                {
                    if (region[4] >= 0) 
                    { 
                        instance.HasDestroyedGeometry = true; 
                        break; 
                    }
                }

                this->CollectLevelStateMap(*hlmt, *instance.Coll, 
                    instance.LevelToState);

                this->CollectDeathStateMap(*hlmt, *instance.Coll,
                    instance.DeathStateMap);

                instance.RegionToSection = 
                    this->CollectRegionToSection(*hlmt, 
                        *instance.Coll);
            }
        }

        instance.CollidableMesh = this->CollectMesh(
            instance, bones, damage);

        instances.push_back(std::move(instance));
    }

    m_Deps.State_Collidables.Publish(std::move(instances));
}

std::vector<std::array<int, 5>> System_Collidables::CollectStateMap(
    const Hlmt_VariantsObject& variant, const CollGeometry& coll)
{
    std::vector<std::array<int, 5>> stateMap(
        coll.RegionNames.size(), 
        std::array<int, 5>{ -1, -1, -1, -1, -1 });

    for (size_t variantRegion = 0; 
        variantRegion < variant.Regions.size(); 
        ++variantRegion)
    {
        const auto& hlmtRegion = variant.Regions[variantRegion];

        int collRegionIdx = -1;
        for (size_t regionName = 0; 
            regionName < coll.RegionNames.size(); 
            ++regionName)
        {
            if (coll.RegionNames[regionName] == hlmtRegion.RegionName)
            {
                collRegionIdx = (int)regionName; 
                break;
            }
        }

        if (collRegionIdx < 0) continue;

        for (const auto& permutation : hlmtRegion.Permutations)
        {
            for (const auto& state : permutation.States)
            {
                if (state.State > 4) continue;

                const uint32_t name = state.PermutationName;
                if (name == 0) continue;

                const auto& names = 
                    coll.PermutationNames[collRegionIdx];

                for (size_t current = 0; 
                    current < names.size(); 
                    ++current)
                {
                    if (names[current] == name)
                    {
                        stateMap[collRegionIdx][state.State] = (int)current; 
                        break;
                    }
                }
            }
        }
    }

    return stateMap;
}

void System_Collidables::CollectLevelStateMap(const HlmtObject& hlmt,
    const CollGeometry& coll, LevelState& levelToState)
{
    levelToState.assign(coll.RegionNames.size(), {});

    for (const auto& damageSection : hlmt.DamageSections)
    {
        int ownRegion = -1;

        for (size_t current = 0; 
            current < coll.RegionNames.size(); 
            ++current)
        {
            if (coll.RegionNames[current] == damageSection.Name) 
            { 
                ownRegion = (int)current; 
                break; 
            }
        }

        if (ownRegion < 0) continue;

        auto& levels = levelToState[ownRegion];
        levels.resize(damageSection.InstantResponses.size(), -1);

        for (size_t instantResponse = 0; 
            instantResponse < damageSection.InstantResponses.size(); 
            ++instantResponse)
        {
            for (const auto& regionTransition : 
                    damageSection.InstantResponses[
                        instantResponse].RegionTransitions)
            {
                if (regionTransition.Region == 
                    damageSection.Name) 
                { 
                    levels[instantResponse] = 
                        (int)regionTransition.NewState; 

                    break; 
                }
            }
        }
    }
}

void System_Collidables::CollectDeathStateMap(const HlmtObject& hlmt, 
    const CollGeometry& coll, std::vector<int>& deathState)
{
    deathState.assign(coll.RegionNames.size(), -1);

    for (const auto& damageSection : hlmt.DamageSections)
    {
        bool isOwnedRegionSection = false;

        for (size_t current = 0; 
            current < coll.RegionNames.size(); 
            ++current)
        {
            if (coll.RegionNames[current] == damageSection.Name) 
            { 
                isOwnedRegionSection = true; 
                break; 
            }
        }

        if (isOwnedRegionSection) continue;

        for (const auto& instantResponse : 
                damageSection.InstantResponses)
        {
            if (instantResponse.DamageThreshold > 0.0001f) continue;

            for (const auto& regionTransition : 
                    instantResponse.RegionTransitions)
            {
                for (size_t current = 0; 
                    current < coll.RegionNames.size(); 
                    ++current)
                {
                    if (coll.RegionNames[current] == 
                        regionTransition.Region)
                    {
                        if ((int)regionTransition.NewState >
                            deathState[current])
                        {
                            deathState[current] = 
                                (int)regionTransition.NewState;
                        }

                        break;
                    }
                }
            }
        }
    }
}

std::vector<int> System_Collidables::CollectRegionToSection(
    const HlmtObject& hlmt, const CollGeometry& coll)
{
    std::vector<int> map(coll.RegionNames.size(), -1);

    for (size_t regionName = 0; 
        regionName < coll.RegionNames.size(); 
        ++regionName)
    {
        for (size_t damageSection = 0; 
            damageSection < hlmt.DamageSections.size(); 
            ++damageSection)
        {
            if (coll.RegionNames[regionName] == 
                hlmt.DamageSections[damageSection].Name)
            {
                map[regionName] = (int)damageSection;
                break;
            }
        }
    }

    return map;
}

CollidableMesh System_Collidables::CollectMesh(
    const Collidable& instance,
    const BoneMatrixTable* bones,
    const DamageSectionTable* damage)
{
    if (!instance.Coll) return {};

    if (bones != nullptr)
    {
        return this->CollectSkeletal(
            instance, *bones, damage);
    }

    // Fallback.
    return this->CollectRigid(instance, damage);
}

CollidableMesh System_Collidables::CollectSkeletal(
    const Collidable& instance, const BoneMatrixTable& bones,
    const DamageSectionTable* damage)
{
    CollidableMesh out;

    if (bones.Matrices.empty() || !instance.Coll)
    {
        return out;
    }

    const auto& matrices = bones.Matrices;

    size_t totalTriangles = 0;
    for (const auto& mesh : instance.Coll->Meshes)
    {
        totalTriangles += mesh.Triangles.size();
    }
    out.Triangles.reserve(totalTriangles);

    for (const auto& mesh : instance.Coll->Meshes)
    {
        if (!this->IsActivePermutation(instance, mesh, damage))
        {
            continue;
        }

        if (mesh.NodeIndex < 0 || 
            static_cast<size_t>(mesh.NodeIndex) >= matrices.size())
        {
            continue;
        }

        const BoneMatrix& bone = matrices[mesh.NodeIndex];

        if (!this->IsBoneMatrixValid(bone)) 
        {
            continue;
        }

        auto poseLocal = [&](const CollVec3& v) {
            const auto& r = bone.Rotation;
            return std::array<float, 3>{
                r[0] * v.X + r[3] * v.Y + r[6] * v.Z + bone.Translation[0],
                r[1] * v.X + r[4] * v.Y + r[7] * v.Z + bone.Translation[1],
                r[2] * v.X + r[5] * v.Y + r[8] * v.Z + bone.Translation[2]
            };
        };

        for (const auto& triangle : mesh.Triangles)
        {
            CollidableTriangle worldTriangle;
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

CollidableMesh System_Collidables::CollectRigid(
    const Collidable& instance,
    const DamageSectionTable* damage)
{
    CollidableMesh out;
    if (!instance.Coll) return out;

    const auto& pos = instance.Position;
    const auto& fwd = instance.Forward;
    const auto& up = instance.Up;
    const auto  rgt = Cross(up, fwd);

    const float cx = (instance.Coll->BoundsMin.X + instance.Coll->BoundsMax.X) * 0.5f;
    const float cy = (instance.Coll->BoundsMin.Y + instance.Coll->BoundsMax.Y) * 0.5f;
    const float cz = (instance.Coll->BoundsMin.Z + instance.Coll->BoundsMax.Z) * 0.5f;

    auto poseLocal = [&](const CollVec3& v) {
        return this->TransformPoint(pos, rgt, fwd, up, 
            v.X - cx, v.Y - cy, v.Z - cz);
    };

    size_t triTotal = 0;
    for (const auto& mesh : instance.Coll->Meshes)
    {
        triTotal += mesh.Triangles.size();
    }
    out.Triangles.reserve(triTotal);


    for (const auto& mesh : instance.Coll->Meshes)
    {
        if (!this->IsActivePermutation(instance, mesh, damage)) continue;

        for (const auto& triangle : mesh.Triangles)
        {
            CollidableTriangle worldTriangle;
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

bool System_Collidables::IsActivePermutation(
    const Collidable& instance, const CollMesh& mesh,
    const DamageSectionTable* damage)
{
    const uint32_t regionName = (instance.Coll &&  
            (size_t)mesh.RegionIndex < instance.Coll->RegionNames.size()) ? 
        instance.Coll->RegionNames[mesh.RegionIndex] : 0;

    auto isDefault = [&](int regionIdx) -> bool {
        if (!instance.Coll || regionIdx < 0 ||
            (size_t)regionIdx >= 
            instance.Coll->DefaultPermutationIndex.size())
        {
            return mesh.PermutationIndex == 0;
        }

        const int def = 
            instance.Coll->DefaultPermutationIndex[regionIdx];

        return def < 0 ? (mesh.PermutationIndex == 0) : 
            (mesh.PermutationIndex == def);
    };

    if (!damage || damage->Sections.empty() || 
        instance.StateMap.empty() || mesh.RegionIndex < 0 ||
        (size_t)mesh.RegionIndex >= instance.StateMap.size() ||
        (size_t)mesh.RegionIndex >= instance.RegionToSection.size())
    {
        return isDefault(mesh.RegionIndex);
    }

    const auto& sections = damage->Sections;
    const auto& row = instance.StateMap[mesh.RegionIndex];
    const int slot = instance.RegionToSection[mesh.RegionIndex];

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
    if ((size_t)mesh.RegionIndex < instance.DeathStateMap.size())
    { 
        deathState = instance.DeathStateMap[mesh.RegionIndex];
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
    if (instance.Coll && mesh.RegionIndex >= 0 &&
        (size_t)mesh.RegionIndex < 
        instance.Coll->DefaultPermutationIndex.size())
    {
        const int def = 
            instance.Coll->DefaultPermutationIndex[mesh.RegionIndex];

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
        (size_t)mesh.RegionIndex < instance.LevelToState.size())
    {
        const auto& levels = instance.LevelToState[mesh.RegionIndex];
        
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

std::array<float, 3> System_Collidables::Cross(
    const std::array<float, 3>& a,
    const std::array<float, 3>& b)
{
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    };
}

std::array<float, 3> System_Collidables::TransformPoint(
    const std::array<float, 3>& pos,
    const std::array<float, 3>& right,
    const std::array<float, 3>& forward,
    const std::array<float, 3>& up,
    float lx, float ly, float lz)
{
    return {
        pos[0] + forward[0] * lx + right[0] * ly + up[0] * lz,
        pos[1] + forward[1] * lx + right[1] * ly + up[1] * lz,
        pos[2] + forward[2] * lx + right[2] * ly + up[2] * lz
    };
}

std::array<float, 3> System_Collidables::TransformByBone(const BoneMatrix& m,
    float lx, float ly, float lz)
{
    const auto& r = m.Rotation;
    const float wx = r[0] * lx + r[1] * ly + r[2] * lz + m.Translation[0];
    const float wy = r[3] * lx + r[4] * ly + r[5] * lz + m.Translation[1];
    const float wz = r[6] * lx + r[7] * ly + r[8] * lz + m.Translation[2];
    return { wx, wy, wz };
}

bool System_Collidables::IsBoneMatrixValid(const BoneMatrix& m)
{
    if (!std::isfinite(m.Scale) || m.Scale < 1e-4f || m.Scale > 1e4f)
    {
        return false;
    }

    for (int row = 0; row < 3; ++row)
    {
        const float a = m.Rotation[row * 3 + 0];
        const float b = m.Rotation[row * 3 + 1];
        const float c = m.Rotation[row * 3 + 2];

        if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c))
        {
            return false;
        }

        const float n2 = a * a + b * b + c * c;
        if (n2 < 0.25f || n2 > 4.0f)
        {
            return false;
        }
    }

    for (int i = 0; i < 3; ++i)
    {
        if (!std::isfinite(m.Translation[i]))
        {
            return false;
        }
    }

    return true;
}

int System_Collidables::HighestLevelFromMask(uint16_t mask)
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

bool System_Collidables::IsAncestorDead(uint32_t handle,
    const ObjectTable& objects)
{
    uint32_t current = m_Deps.State_ObjectGraph.GetParent(handle);
    int guard = 0;

    while (current != 0 && current != 0xFFFFFFFF && guard++ < 16)
    {
        auto it = objects.find(current);
        if (it != objects.end() && it->second.Address != 0)
        {
            const DamageSectionTable* dmg =
                m_Deps.State_DamageSections.Get(current);

            if (dmg && !dmg->Sections.empty() &&
                dmg->Sections[0].Vitality <= 0.0f)
            {
                return true;
            }
        }

        const uint32_t parent = m_Deps.State_ObjectGraph.GetParent(current);
        if (parent == current) break;
        current = parent;
    }

    return false;
}

void System_Collidables::Cleanup()
{
    m_Deps.State_Collidables.Cleanup();
    m_Deps.System_Logs.Log("[World] INFO: Cleanup completed.");
}

void System_Collidables::LogDamageStructure(
    const HlmtObject& hlmt, const CollGeometry& coll)
{
    for (size_t ds = 0; ds < hlmt.DamageSections.size(); ++ds)
    {
        const auto& section = hlmt.DamageSections[ds];

        m_Deps.System_Logs.Log(
            "[DmgStruct] section=%zu name=0x%X vitality%%=%.3f "
            "instantResponses=%zu",
            ds, section.Name, section.VitalityPercentage,
            section.InstantResponses.size());

        for (size_t ir = 0; ir < section.InstantResponses.size(); ++ir)
        {
            const auto& resp = section.InstantResponses[ir];

            m_Deps.System_Logs.Log(
                "[DmgStruct]   resp=%zu threshold=%.3f totalThreshold=%.3f "
                "regionTransitions=%zu",
                ir, resp.DamageThreshold, resp.TotalDamageThreshold,
                resp.RegionTransitions.size());

            m_Deps.System_Logs.Log(
                "[DmgStruct]   resp=%zu threshold=%.3f flags=0x%X destroyInstGroup=%d "
                "regionTransitions=%zu",
                ir, resp.DamageThreshold, resp.Flags,
                (int)resp.DestroyInstanceGroupIndex,
                resp.RegionTransitions.size());

            for (size_t rt = 0; rt < resp.RegionTransitions.size(); ++rt)
            {
                const auto& trans = resp.RegionTransitions[rt];

                // Resolve which coll region this transition targets.
                int collReg = -1;
                for (size_t cr = 0; cr < coll.RegionNames.size(); ++cr)
                    if (coll.RegionNames[cr] == trans.Region) { collReg = (int)cr; break; }

                m_Deps.System_Logs.Log(
                    "[DmgStruct]     trans=%zu region=0x%X (collReg=%d) "
                    "newState=%d runtimeRegionIdx=%d",
                    rt, trans.Region, collReg,
                    (int)trans.NewState, (int)trans.RuntimeRegionIndex);
            }
        }
    }
}