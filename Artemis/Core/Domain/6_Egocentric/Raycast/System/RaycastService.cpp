module;

#include <cmath>

module Egocentric.Raycast.System;

import Common.Math.System;
import Common.Geometry.System;

namespace
{
	using RaycastHit = Egocentric::Raycast::Type::RaycastHit;
	using HitKind = Egocentric::Raycast::Type::HitKind;

	using Egocentric::Raycast::Type::Constant::k_FallbackAimRange;
	using Egocentric::Raycast::Type::Constant::k_MaxRayRange;
	using Egocentric::Raycast::Type::Constant::k_DynamicRejectMargin;
	using Egocentric::Raycast::Type::Constant::k_PerceptionRange;
	using Egocentric::Raycast::Type::Constant::k_PerceptionRayCount;

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
}

namespace Egocentric::Raycast::System
{
	auto RaycastService::Update() -> void
	{
		auto selfPtr = m_SelfStore.Acquire();
		if (!selfPtr || !selfPtr->IsAlive) return;

		Raycasts result{};

		const float aimRange = this->ResolveAimRange(selfPtr->Handle);

		const SelfExclusion selfExclusion =
			this->BuildSelfExclusion(selfPtr->Handle, selfPtr->BipedHandle);

		const PerceptionOrigin perceptionOrigin =
			this->ResolvePerceptionOrigin(selfPtr->Handle, selfPtr->BipedHandle);

		const Vec3 originPosition = perceptionOrigin.Valid ? 
			perceptionOrigin.Position : selfPtr->Position;

		const Vec3 originForwardIn = perceptionOrigin.Valid ? 
			perceptionOrigin.Forward : selfPtr->Forward;

		Vec3 originForward{}, originRight{}, originUp{};
		Common::Math::System::BuildFrame(originForwardIn, originForward, originRight, originUp);

		const float cameraToOriginDistance = Common::Math::System::Distance(
			selfPtr->Position, originPosition);

		const float dynamicCacheRadius =
			(std::max)(aimRange, k_PerceptionRange) + cameraToOriginDistance;
		const DynamicCache dynamicCache = this->BuildDynamicCache(
			selfPtr->Position, dynamicCacheRadius, selfExclusion);

		result.AimHit = this->CastRay(selfPtr->Position, selfPtr->Forward, aimRange, dynamicCache);

		result.PerceptionHits.reserve(k_PerceptionRayCount);

		for (const Vec3& local : k_PerceptionCubeLocalDirections)
		{
			const Vec3 direction{
				originRight.X * local.X + originUp.X * local.Y + originForward.X * local.Z,
				originRight.Y * local.X + originUp.Y * local.Y + originForward.Y * local.Z,
				originRight.Z * local.X + originUp.Z * local.Y + originForward.Z * local.Z
			};

			result.PerceptionHits.push_back(
				this->CastRay(originPosition, direction, k_PerceptionRange, dynamicCache));
		}

		m_RaycastStore.Publish(std::move(result));
	}

	auto RaycastService::ResolveAimRange(std::uint32_t selfPlayerHandle) const -> float
	{
		float resolved = this->ResolveAimRangeUncapped(selfPlayerHandle);

		return (std::min)(resolved, k_MaxRayRange);
	}

	auto RaycastService::ResolveAimRangeUncapped(std::uint32_t selfPlayerHandle) const -> float
	{
		auto playerTablePtr = m_PlayerStore.Acquire();
		if (!playerTablePtr) return k_FallbackAimRange;

		auto playerIt = playerTablePtr->find(selfPlayerHandle);
		if (playerIt == playerTablePtr->end()) return k_FallbackAimRange;

		const std::uint32_t weaponHandle = playerIt->second.PrimaryWeaponHandle;
		if (weaponHandle == 0 || weaponHandle == 0xFFFFFFFF) return k_FallbackAimRange;

		auto objectTablePtr = m_ObjectStore.Acquire();
		if (!objectTablePtr) return k_FallbackAimRange;

		auto weaponIt = objectTablePtr->find(weaponHandle);
		if (weaponIt == objectTablePtr->end()) return k_FallbackAimRange;

		const ResolvedWeap* weap = m_StatsStore.GetResolvedWeap(weaponIt->second.TagName);
		if (!weap || weap->AutoaimRange <= 0.0f) return k_FallbackAimRange;

		return weap->AutoaimRange;
	}

	auto RaycastService::BuildSelfExclusion(std::uint32_t selfPlayerHandle,
		std::uint32_t selfBipedHandle) const -> SelfExclusion
	{
		SelfExclusion exclusion;
		if (selfBipedHandle == 0xFFFFFFFF) return exclusion;

		exclusion.insert(selfBipedHandle);

		auto playerGraphPtr = m_PlayerGraphStore.Acquire();
		if (playerGraphPtr)
		{
			for (const PlayerTree& tree : *playerGraphPtr)
			{
				if (tree.Handle != selfPlayerHandle) continue;

				if (tree.PrimaryWeaponHandle != 0xFFFFFFFF) exclusion.insert(tree.PrimaryWeaponHandle);
				if (tree.SecondaryWeaponHandle != 0xFFFFFFFF) exclusion.insert(tree.SecondaryWeaponHandle);
				if (tree.AbilityHandle != 0xFFFFFFFF) exclusion.insert(tree.AbilityHandle);
				if (tree.ObjectiveHandle != 0xFFFFFFFF) exclusion.insert(tree.ObjectiveHandle);

				if (tree.IsInVehicle())
				{
					exclusion.insert(tree.ParentHandle);
					for (std::uint32_t part : tree.VehiclePartHandles) exclusion.insert(part);
				}

				break;
			}
		}

		auto objectTablePtr = m_ObjectStore.Acquire();
		if (objectTablePtr)
		{
			for (const auto& [handle, object] : *objectTablePtr)
			{
				if (object.ParentHandle == selfBipedHandle) exclusion.insert(handle);
			}
		}

		return exclusion;
	}

	auto RaycastService::ResolvePerceptionOrigin(std::uint32_t selfPlayerHandle,
		std::uint32_t selfBipedHandle) const -> PerceptionOrigin
	{
		std::uint32_t targetHandle = selfBipedHandle;

		auto playerGraphPtr = m_PlayerGraphStore.Acquire();
		if (playerGraphPtr)
		{
			for (const PlayerTree& tree : *playerGraphPtr)
			{
				if (tree.Handle != selfPlayerHandle) continue;
				if (tree.IsInVehicle()) targetHandle = tree.ParentHandle;
				break;
			}
		}

		if (targetHandle == 0xFFFFFFFF) return {};

		auto objectTablePtr = m_ObjectStore.Acquire();
		if (!objectTablePtr) return {};

		auto objectIt = objectTablePtr->find(targetHandle);
		if (objectIt == objectTablePtr->end()) return {};

		return { objectIt->second.Position, objectIt->second.Forward, true };
	}

	auto RaycastService::BuildDynamicCache(const Vec3& origin, float radius,
		const SelfExclusion& selfExclusion) -> DynamicCache
	{
		DynamicCache cache;

		const auto candidates = m_CollidableService.QueryNearby(origin, radius);
		cache.reserve(candidates.size());

		for (std::uint32_t handle : candidates)
		{
			if (selfExclusion.contains(handle)) continue;

			auto collidable = m_CollidableService.CollectMeshFor(handle);
			if (!collidable) continue;

			float boundingRadiusSq = 0.0f;
			for (const Triangle& triangle : collidable->WorldMesh.Triangles)
			{
				for (const Vec3& vertex : { triangle.A, triangle.B, triangle.C })
				{
					const float dx = vertex.X - collidable->Position.X;
					const float dy = vertex.Y - collidable->Position.Y;
					const float dz = vertex.Z - collidable->Position.Z;
					boundingRadiusSq = (std::max)(boundingRadiusSq, dx * dx + dy * dy + dz * dz);
				}
			}

			CachedDynamic entry{};
			entry.BoundingRadius = std::sqrt(boundingRadiusSq) + k_DynamicRejectMargin;
			entry.Data = std::move(*collidable);

			cache.emplace(handle, std::move(entry));
		}

		return cache;
	}

	auto RaycastService::CastRay(const Vec3& origin, const Vec3& direction, float maxDistance,
		const DynamicCache& dynamicCache) const -> RaycastHit
	{
		RaycastHit best{};
		best.Origin = origin;
		best.Direction = direction;
		best.MaxDistance = maxDistance;

		const auto staticHit = m_SbspRaycaster.Cast(m_WorldStore, origin, direction, maxDistance);
		if (staticHit.IsHit)
		{
			best.Hit = true;
			best.Kind = HitKind::Static;
			best.Distance = staticHit.Distance;
			best.Point = staticHit.Point;
		}

		const float dynamicSearchRadius = best.Hit ? best.Distance : maxDistance;

		for (const auto& [handle, entry] : dynamicCache)
		{
			const auto& collidable = entry.Data;

			const float px = collidable.Position.X - origin.X;
			const float py = collidable.Position.Y - origin.Y;
			const float pz = collidable.Position.Z - origin.Z;

			float tAlong = px * direction.X + py * direction.Y + pz * direction.Z;
			tAlong = (std::max)(0.0f, (std::min)(tAlong, dynamicSearchRadius));

			const float cx = px - direction.X * tAlong;
			const float cy = py - direction.Y * tAlong;
			const float cz = pz - direction.Z * tAlong;

			const float rejectRadius = entry.BoundingRadius;
			if (cx * cx + cy * cy + cz * cz > rejectRadius * rejectRadius) continue;

			for (const Triangle& triangle : collidable.WorldMesh.Triangles)
			{
				float distance = 0.0f;
				const float currentBest = best.Hit ? best.Distance : maxDistance;

				if (!Common::Geometry::System::RayIntersectsTriangle(
					origin, direction, triangle, currentBest, distance))
				{
					continue;
				}

				if (!best.Hit || distance < best.Distance)
				{
					best.Hit = true;
					best.Kind = HitKind::Dynamic;
					best.Distance = distance;
					best.ObjectHandle = handle;
					best.Point = {
						origin.X + direction.X * distance,
						origin.Y + direction.Y * distance,
						origin.Z + direction.Z * distance
					};
				}
			}
		}

		return best;
	}

	auto RaycastService::Cleanup() -> void
	{
		m_RaycastStore.Cleanup();
		m_LogsService.Message("[RaycastService] INFO: Cleanup completed.");
	}
}