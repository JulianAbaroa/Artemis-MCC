module;

#include <cmath>

module Egocentric.Raycast.System;

import Common.Math.System;
import Common.Geometry.System;

namespace
{
	using RaycastHit = Egocentric::Raycast::Type::RaycastHit;
	using CachedPart = Egocentric::Raycast::Type::CachedPart;
	using CollidablePart = Environment::Collidable::Type::CollidablePart;
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

	// Padding of the boxes, so rays that graze a triangle edge are not lost to rounding.
	constexpr float k_BoxPadding{ 1.0e-3f };

	// Prepares the parts of an object to be hit by rays.
	// note: Computed once per tick for each object, instead of posing every triangle.
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

	// Whether a ray passes within a radius of a point, before travelling limit.
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

	// Whether a ray crosses a box before travelling tMax.
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

		const ResolvedWeap* weap = m_DefinitionsStore.Weap.Get(weaponIt->second.TagName);
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

			auto collidable = m_CollidableService.CollectPartsFor(handle);
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

	auto RaycastService::CastRay(const Vec3& origin, const Vec3& direction, float maxDistance,
		const DynamicCache& dynamicCache) const -> RaycastHit
	{
		RaycastHit best{};
		best.Origin = origin;
		best.Direction = direction;
		best.MaxDistance = maxDistance;

		const auto staticHit = m_Raycaster.Cast(origin, direction, maxDistance);
		if (staticHit.IsHit)
		{
			best.Hit = true;
			best.Kind = HitKind::Static;
			best.Distance = staticHit.Distance;
			best.Point = staticHit.Point;
		}

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

		return best;
	}

	auto RaycastService::Cleanup() -> void
	{
		m_RaycastStore.Cleanup();
		m_LogsService.Message("[RaycastService] INFO: Cleanup completed.");
	}
}