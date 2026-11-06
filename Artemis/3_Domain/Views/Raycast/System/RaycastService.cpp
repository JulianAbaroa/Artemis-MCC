module;

#include <cmath>

module Views.Raycast.System;

import Common.Engine.Type;
import Common.Math.System;
import Common.Geometry.System;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.Type;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Object.Type;
import Perception.ObjectGraph.Type;
import Perception.Entities.Type;
import Perception.Players.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Triangle;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::CachedDynamic;
    using Views::Raycast::Type::DynamicCache;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::PerceptionOrigin;
    using Views::Raycast::Type::Raycasts;
    using Views::Raycast::Type::SelfExclusion;
    using Perception::Collidable::Type::CollidablePart;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::MapModel::Definitions::Type::Weap::Weap;
    using Perception::Entities::Type::Entity;
    using Perception::Entities::Type::Entities;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Loadout;

    using Perception::Collidable::System::CollidableService;
    using Blam::MapModel::Definitions::State::DefinitionsStore;
    using Perception::Entities::State::EntitiesStore;
    using Blam::Runtime::Object::State::ObjectTableStore;
    using Perception::Players::State::PlayersStore;
    using Blam::Runtime::Player::State::PlayerTableStore;
    using Blam::MapModel::World::System::Raycaster;

    using Views::Raycast::Type::Constant::k_FallbackAimRange;
    using Views::Raycast::Type::Constant::k_MaxRayRange;
    using Views::Raycast::Type::Constant::k_DynamicRejectMargin;
    using Views::Raycast::Type::Constant::k_PerceptionRange;
    using Views::Raycast::Type::Constant::k_PerceptionRayCount;
    using Blam::MapModel::World::Type::Raycast::k_BoxPadding;
    using Perception::ObjectGraph::Type::k_MaxParentDepth;
    using Common::Engine::Type::k_NoHandle;

    constexpr float k_InvSqrt3 = 0.57735026918962576451f;

    constexpr std::array<Common::Math::Type::Vec3, 14> k_PerceptionCubeLocalDirections{ {
            {  1.0f,  0.0f,  0.0f },
            { -1.0f,  0.0f,  0.0f },
            {  0.0f,  1.0f,  0.0f },
            {  0.0f, -1.0f,  0.0f },
            {  0.0f,  0.0f,  1.0f },
            {  0.0f,  0.0f, -1.0f },
            {  k_InvSqrt3,  k_InvSqrt3,  k_InvSqrt3 },
            {  k_InvSqrt3,  k_InvSqrt3, -k_InvSqrt3 },
            {  k_InvSqrt3, -k_InvSqrt3,  k_InvSqrt3 },
            {  k_InvSqrt3, -k_InvSqrt3, -k_InvSqrt3 },
            { -k_InvSqrt3,  k_InvSqrt3,  k_InvSqrt3 },
            { -k_InvSqrt3,  k_InvSqrt3, -k_InvSqrt3 },
            { -k_InvSqrt3, -k_InvSqrt3,  k_InvSqrt3 },
            { -k_InvSqrt3, -k_InvSqrt3, -k_InvSqrt3 },
    } };

    // Returns the vehicle at the root of the parent chain of a biped, or 0xFFFFFFFF when it rides none.
    // note: A turret is a vehicle attached to another one, so the root is the vehicle that carries every part.
    auto FindVehicleRoot(const Entities& entities, std::uint32_t bipedHandle) -> std::uint32_t
    {
        const Entity* current = entities.Find(bipedHandle);
        if (!current || current->ParentHandle == k_NoHandle) return k_NoHandle;

        for (int depth{ 0 }; depth < k_MaxParentDepth && current->ParentHandle != k_NoHandle; ++depth)
        {
            const Entity* parent = entities.Find(current->ParentHandle);
            if (!parent) break;

            current = parent;
        }

        return current->Kind == ObjectKind::Vehicle ? current->Handle : k_NoHandle;
    }

    // return: The aim range of the weapon of the player without the cap, or the fallback range if it has none.
    auto ResolveAimRangeUncapped(const PlayerTableStore& playerStore, const ObjectTableStore& objectStore,
        const DefinitionsStore& definitionsStore, std::uint32_t selfPlayerHandle) -> float
    {
        auto playerTablePtr = playerStore.Acquire();
        if (!playerTablePtr) return k_FallbackAimRange;

        auto playerIt = playerTablePtr->find(selfPlayerHandle);
        if (playerIt == playerTablePtr->end()) return k_FallbackAimRange;

        const std::uint32_t weaponHandle = playerIt->second.PrimaryWeaponHandle;
        if (weaponHandle == 0 || weaponHandle == k_NoHandle) return k_FallbackAimRange;

        auto objectTablePtr = objectStore.Acquire();
        if (!objectTablePtr) return k_FallbackAimRange;

        auto weaponIt = objectTablePtr->find(weaponHandle);
        if (weaponIt == objectTablePtr->end()) return k_FallbackAimRange;

        const Weap* weap = definitionsStore.m_Weap.Get(weaponIt->second.TagName);
        if (!weap || weap->AutoaimRange <= 0.0f) return k_FallbackAimRange;

        return weap->AutoaimRange;
    }
}

namespace Views::Raycast::System
{
    auto PerceptionDirections(const Vec3& forward) -> std::array<Vec3, k_PerceptionRayCount>
    {
        Vec3 originForward{}, originRight{}, originUp{};
        Common::Math::System::BuildFrame(forward, originForward, originRight, originUp);

        std::array<Vec3, k_PerceptionRayCount> directions{};
        for (std::size_t index{ 0 }; index < directions.size(); ++index)
        {
            const Vec3& local = k_PerceptionCubeLocalDirections[index];
            directions[index] = Vec3{
                originRight.X * local.X + originUp.X * local.Y + originForward.X * local.Z,
                originRight.Y * local.X + originUp.Y * local.Y + originForward.Y * local.Z,
                originRight.Z * local.X + originUp.Z * local.Y + originForward.Z * local.Z
            };
        }

        return directions;
    }

    auto DynamicCacheRadius(float aimRange, float cameraToOriginDistance) -> float
    {
        return (std::max)(aimRange, k_PerceptionRange) + cameraToOriginDistance;
    }

    auto ResolveAimRange(const PlayerTableStore& playerStore, const ObjectTableStore& objectStore,
        const DefinitionsStore& definitionsStore, std::uint32_t selfPlayerHandle) -> float
    {
        float resolved = ResolveAimRangeUncapped(playerStore, objectStore, definitionsStore, selfPlayerHandle);

        return (std::min)(resolved, k_MaxRayRange);
    }

    auto BuildSelfExclusion(const PlayersStore& playersStore, const EntitiesStore& entitiesStore,
        const ObjectTableStore& objectStore, std::uint32_t selfPlayerHandle, std::uint32_t selfBipedHandle) -> SelfExclusion
    {
        SelfExclusion exclusion;
        if (selfBipedHandle == k_NoHandle) return exclusion;

        exclusion.insert(selfBipedHandle);

        auto playersPtr = playersStore.Acquire();
        const Player* player = playersPtr ? playersPtr->Find(selfPlayerHandle) : nullptr;
        if (player && player->IsAlive())
        {
            const Loadout& loadout = player->Body->Loadout;

            for (const std::uint32_t handle : { loadout.PrimaryWeapon, loadout.SecondaryWeapon, loadout.Ability, loadout.Objective })
            {
                if (handle != k_NoHandle) exclusion.insert(handle);
            }
        }

        auto entitiesPtr = entitiesStore.Acquire();
        if (entitiesPtr)
        {
            const std::uint32_t vehicleHandle = FindVehicleRoot(*entitiesPtr, selfBipedHandle);
            if (vehicleHandle != k_NoHandle)
            {
                exclusion.insert(vehicleHandle);

                for (const Entity& entity : entitiesPtr->Rows)
                {
                    if (entity.Kind == ObjectKind::Vehicle && entity.ParentHandle == vehicleHandle) exclusion.insert(entity.Handle);
                }
            }
        }

        auto objectTablePtr = objectStore.Acquire();
        if (objectTablePtr)
        {
            for (const auto& [handle, object] : *objectTablePtr)
            {
                if (object.ParentHandle == selfBipedHandle) exclusion.insert(handle);
            }
        }

        return exclusion;
    }

    auto ResolvePerceptionOrigin(const EntitiesStore& entitiesStore, const ObjectTableStore& objectStore,
        std::uint32_t selfBipedHandle) -> PerceptionOrigin
    {
        std::uint32_t targetHandle = selfBipedHandle;

        auto entitiesPtr = entitiesStore.Acquire();
        if (entitiesPtr && selfBipedHandle != k_NoHandle)
        {
            const std::uint32_t vehicleHandle = FindVehicleRoot(*entitiesPtr, selfBipedHandle);
            if (vehicleHandle != k_NoHandle) targetHandle = vehicleHandle;
        }

        if (targetHandle == k_NoHandle) return {};

        auto objectTablePtr = objectStore.Acquire();
        if (!objectTablePtr) return {};

        auto objectIt = objectTablePtr->find(targetHandle);
        if (objectIt == objectTablePtr->end()) return {};

        return { objectIt->second.BoundingCenter, objectIt->second.Forward, true };
    }

    auto PrepareParts(const std::vector<CollidablePart>& parts) -> std::vector<CachedPart>
    {
        std::vector<CachedPart> out{};
        out.reserve(parts.size());

        for (const CollidablePart& part : parts)
        {
            if (!part.Source || part.Source->Triangles.empty()) continue;

            const auto& m = part.Transform;

            const float a = m[0], b = m[1], c = m[2];
            const float d = m[4], e = m[5], f = m[6];
            const float g = m[8], h = m[9], i = m[10];

            const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
            if (std::fabs(det) < 1.0e-8f) continue;
            const float inv = 1.0f / det;

            const float i00 = (e * i - f * h) * inv;
            const float i01 = (c * h - b * i) * inv;
            const float i02 = (b * f - c * e) * inv;
            const float i10 = (f * g - d * i) * inv;
            const float i11 = (a * i - c * g) * inv;
            const float i12 = (c * d - a * f) * inv;
            const float i20 = (d * h - e * g) * inv;
            const float i21 = (b * g - a * h) * inv;
            const float i22 = (a * e - b * d) * inv;

            CachedPart cached{};
            cached.Source = part.Source;
            cached.ToLocal = {
                i00, i01, i02, -(i00 * m[3] + i01 * m[7] + i02 * m[11]),
                i10, i11, i12, -(i10 * m[3] + i11 * m[7] + i12 * m[11]),
                i20, i21, i22, -(i20 * m[3] + i21 * m[7] + i22 * m[11])
            };

            // The box is posed through its eight corners, so the sphere holds for any transform.
            const auto& lo = part.Source->LocalMin;
            const auto& hi = part.Source->LocalMax;
            auto toWorld = [&](float x, float y, float z) -> Common::Math::Type::Vec3 {
                return {
                    m[0] * x + m[1] * y + m[2] * z + m[3],
                    m[4] * x + m[5] * y + m[6] * z + m[7],
                    m[8] * x + m[9] * y + m[10] * z + m[11]
                };
            };

            cached.Center = toWorld((lo.X + hi.X) * 0.5f, (lo.Y + hi.Y) * 0.5f, (lo.Z + hi.Z) * 0.5f);

            float radiusSq{};
            for (const float x : { lo.X, hi.X })
            {
                for (const float y : { lo.Y, hi.Y })
                {
                    for (const float z : { lo.Z, hi.Z })
                    {
                        const auto corner = toWorld(x, y, z);
                        const float dx = corner.X - cached.Center.X;
                        const float dy = corner.Y - cached.Center.Y;
                        const float dz = corner.Z - cached.Center.Z;
                        radiusSq = (std::max)(radiusSq, dx * dx + dy * dy + dz * dz);
                    }
                }
            }

            cached.Radius = std::sqrt(radiusSq);
            out.push_back(cached);
        }

        return out;
    }

    auto ArmDynamicCache(CollidableService& collidableService, const std::vector<std::uint32_t>& candidates,
        const SelfExclusion& selfExclusion) -> DynamicCache
    {
        DynamicCache cache;
        cache.reserve(candidates.size());

        for (std::uint32_t handle : candidates)
        {
            if (selfExclusion.contains(handle)) continue;

            auto collidable = collidableService.CollectPartsFor(handle);
            if (!collidable) continue;

            CachedDynamic entry{};
            entry.Parts = PrepareParts(collidable->Parts);

            float boundingRadius{};
            for (const CachedPart& part : entry.Parts)
            {
                const float dx = part.Center.X - collidable->Position.X;
                const float dy = part.Center.Y - collidable->Position.Y;
                const float dz = part.Center.Z - collidable->Position.Z;
                boundingRadius = (std::max)(boundingRadius, std::sqrt(dx * dx + dy * dy + dz * dz) + part.Radius);
            }

            entry.BoundingRadius = boundingRadius + k_DynamicRejectMargin;
            entry.Data = std::move(*collidable);

            cache.emplace(handle, std::move(entry));
        }

        return cache;
    }

    auto BuildDynamicCache(CollidableService& collidableService, const Vec3& origin, float radius,
        const SelfExclusion& selfExclusion) -> DynamicCache
    {
        return ArmDynamicCache(collidableService, collidableService.QueryNearby(origin, radius), selfExclusion);
    }

    auto RayNearPoint(const Common::Math::Type::Vec3& origin, const Common::Math::Type::Vec3& direction,
        float limit, const Common::Math::Type::Vec3& center, float radius) -> bool
    {
        const float px = center.X - origin.X;
        const float py = center.Y - origin.Y;
        const float pz = center.Z - origin.Z;

        float along = px * direction.X + py * direction.Y + pz * direction.Z;
        along = (std::max)(0.0f, (std::min)(along, limit));

        const float cx = px - direction.X * along;
        const float cy = py - direction.Y * along;
        const float cz = pz - direction.Z * along;

        return cx * cx + cy * cy + cz * cz <= radius * radius;
    }

    auto RayHitsBox(const Common::Math::Type::Vec3& origin, const Common::Math::Type::Vec3& direction,
        const Common::Math::Type::Vec3& boxMin, const Common::Math::Type::Vec3& boxMax, float tMax) -> bool
    {
        const std::array<float, 3> o{ origin.X, origin.Y, origin.Z };
        const std::array<float, 3> dir{ direction.X, direction.Y, direction.Z };
        const std::array<float, 3> lo{ boxMin.X - k_BoxPadding, boxMin.Y - k_BoxPadding, boxMin.Z - k_BoxPadding };
        const std::array<float, 3> hi{ boxMax.X + k_BoxPadding, boxMax.Y + k_BoxPadding, boxMax.Z + k_BoxPadding };

        float t0{ 0.0f };
        float t1{ tMax };
        for (int axis = 0; axis < 3; ++axis)
        {
            const float inverse = std::fabs(dir[axis]) < 1.0e-20f ? 1.0e20f : 1.0f / dir[axis];
            float ta = (lo[axis] - o[axis]) * inverse;
            float tb = (hi[axis] - o[axis]) * inverse;
            if (ta > tb) std::swap(ta, tb);
            t0 = (std::max)(t0, ta);
            t1 = (std::min)(t1, tb);
            if (t0 > t1) return false;
        }

        return true;
    }

    auto CastStatic(const Raycaster& raycaster, const Vec3& origin, const Vec3& direction, float maxDistance) -> RaycastHit
    {
        RaycastHit best{};
        best.Origin = origin;
        best.Direction = direction;
        best.MaxDistance = maxDistance;

        const auto staticHit = raycaster.Cast(origin, direction, maxDistance);
        if (staticHit.IsHit)
        {
            best.Hit = true;
            best.Kind = HitKind::Static;
            best.Distance = staticHit.Distance;
            best.Point = staticHit.Point;
        }

        return best;
    }

    auto CastDynamic(const DynamicCache& dynamicCache, const Vec3& origin, const Vec3& direction, float maxDistance,
        RaycastHit& best) -> void
    {
        for (const auto& [handle, entry] : dynamicCache)
        {
            const auto& collidable = entry.Data;

            float limit = best.Hit ? best.Distance : maxDistance;

            if (!RayNearPoint(origin, direction, limit, collidable.Position, entry.BoundingRadius)) continue;

            for (const CachedPart& part : entry.Parts)
            {
                limit = best.Hit ? best.Distance : maxDistance;

                if (!RayNearPoint(origin, direction, limit, part.Center, part.Radius)) continue;

                const auto& m = part.ToLocal;
                const Vec3 localOrigin{
                    m[0] * origin.X + m[1] * origin.Y + m[2] * origin.Z + m[3],
                    m[4] * origin.X + m[5] * origin.Y + m[6] * origin.Z + m[7],
                    m[8] * origin.X + m[9] * origin.Y + m[10] * origin.Z + m[11]
                };
                const Vec3 localDirection{
                    m[0] * direction.X + m[1] * direction.Y + m[2] * direction.Z,
                    m[4] * direction.X + m[5] * direction.Y + m[6] * direction.Z,
                    m[8] * direction.X + m[9] * direction.Y + m[10] * direction.Z
                };

                if (!RayHitsBox(localOrigin, localDirection, part.Source->LocalMin, part.Source->LocalMax, limit)) continue;

                for (const Triangle& triangle : part.Source->Triangles)
                {
                    float distance = 0.0f;
                    const float currentBest = best.Hit ? best.Distance : maxDistance;

                    if (!Common::Geometry::System::RayIntersectsTriangle(
                        localOrigin, localDirection, triangle, currentBest, distance))
                    {
                        continue;
                    }

                    if (!best.Hit || distance < best.Distance)
                    {
                        best.Hit = true;
                        best.Kind = HitKind::Dynamic;
                        best.Distance = distance;
                        best.ObjectHandle = handle;
                        best.RegionIndex = part.Source->RegionIndex;
                        best.Point = {
                            origin.X + direction.X * distance,
                            origin.Y + direction.Y * distance,
                            origin.Z + direction.Z * distance
                        };
                    }
                }
            }
        }
    }

    auto CastRay(const Raycaster& raycaster, const DynamicCache& dynamicCache, const Vec3& origin, const Vec3& direction,
        float maxDistance) -> RaycastHit
    {
        RaycastHit best = CastStatic(raycaster, origin, direction, maxDistance);
        CastDynamic(dynamicCache, origin, direction, maxDistance, best);

        return best;
    }
}

namespace Views::Raycast::System
{
    auto RaycastService::Update() -> void
    {
        auto selfPtr = m_SelfStore.Acquire();
        if (!selfPtr || !selfPtr->IsAlive) return;

        Raycasts result{};

        const float aimRange = ResolveAimRange(m_PlayerStore, m_ObjectStore, m_DefinitionsStore, selfPtr->Handle);

        const SelfExclusion selfExclusion = BuildSelfExclusion(m_PlayersStore, m_EntitiesStore, m_ObjectStore,
            selfPtr->Handle, selfPtr->BipedHandle);

        const PerceptionOrigin perceptionOrigin = ResolvePerceptionOrigin(m_EntitiesStore, m_ObjectStore, selfPtr->BipedHandle);

        const Vec3 originPosition = perceptionOrigin.Valid ?
            perceptionOrigin.Position : selfPtr->Position;

        const Vec3 originForward = perceptionOrigin.Valid ?
            perceptionOrigin.Forward : selfPtr->Forward;

        const float cameraToOriginDistance = Common::Math::System::Distance(
            selfPtr->Position, originPosition);

        const DynamicCache dynamicCache = BuildDynamicCache(m_CollidableService, selfPtr->Position,
            DynamicCacheRadius(aimRange, cameraToOriginDistance), selfExclusion);

        result.AimHit = CastRay(m_Raycaster, dynamicCache, selfPtr->Position, selfPtr->Forward, aimRange);

        result.PerceptionHits.reserve(k_PerceptionRayCount);

        for (const Vec3& direction : PerceptionDirections(originForward))
        {
            result.PerceptionHits.push_back(CastRay(m_Raycaster, dynamicCache, originPosition, direction, k_PerceptionRange));
        }

        m_RaycastStore.Publish(std::move(result));
    }

    auto RaycastService::Cleanup() -> void
    {
        m_RaycastStore.Cleanup();
        m_Log.Message("[RaycastService] INFO: Cleanup completed.");
    }
}