module;

#include <cmath>

module Resolved.World.System;
import :Raycast;

import Common.Geometry.System;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using ResolvedSbsp = Resolved::World::Type::Sbsp::Sbsp;

	using Resolved::World::Type::Constant::k_CellSize;

	auto FloorToCell(float value, float cellSize) -> std::int32_t
	{
		return static_cast<std::int32_t>(std::floor(value / cellSize));
	}

	auto SafeDiv(float num, float den) -> float
	{
		if (den == 0.0f) return (std::numeric_limits<float>::max)();
		return num / den;
	}
}

namespace Resolved::World::System
{
	auto SbspRaycaster::CellKeyFor(const Vec3& point) const -> CellKey
	{
		return {
			FloorToCell(point.X, k_CellSize),
			FloorToCell(point.Y, k_CellSize),
			FloorToCell(point.Z, k_CellSize)
		};
	}

	auto SbspRaycaster::EnsureGrid(const WorldStore& worldStore) -> void
	{
		if (m_GridBuilt) return;

		m_Cells.clear();

		const auto& sbsps = worldStore.GetAllResolvedSbsps();

		for (std::size_t sbspIndex = 0; sbspIndex < sbsps.size(); ++sbspIndex)
		{
			const auto& triangles = sbsps[sbspIndex].RenderGeometry;

			for (std::size_t triIndex = 0; triIndex < triangles.size(); ++triIndex)
			{
				const auto& tri = triangles[triIndex];

				const float minX = (std::min)({ tri.A.X, tri.B.X, tri.C.X });
				const float minY = (std::min)({ tri.A.Y, tri.B.Y, tri.C.Y });
				const float minZ = (std::min)({ tri.A.Z, tri.B.Z, tri.C.Z });
				const float maxX = (std::max)({ tri.A.X, tri.B.X, tri.C.X });
				const float maxY = (std::max)({ tri.A.Y, tri.B.Y, tri.C.Y });
				const float maxZ = (std::max)({ tri.A.Z, tri.B.Z, tri.C.Z });

				const CellKey cellMin = this->CellKeyFor({ minX, minY, minZ });
				const CellKey cellMax = this->CellKeyFor({ maxX, maxY, maxZ });

				const TriangleRef ref {
					static_cast<std::int32_t>(sbspIndex),
					static_cast<std::int32_t>(triIndex)
				};

				for (std::int32_t x = cellMin.X; x <= cellMax.X; ++x)
				{
					for (std::int32_t y = cellMin.Y; y <= cellMax.Y; ++y)
					{
						for (std::int32_t z = cellMin.Z; z <= cellMax.Z; ++z)
						{
							m_Cells[CellKey{ x, y, z }].push_back(ref);
						}
					}
				}
			}
		}

		m_GridBuilt = true;
	}

	auto SbspRaycaster::Cast(const WorldStore& worldStore, const Vec3& origin,
		const Vec3& direction, float maxDistance) -> Hit
	{
		this->EnsureGrid(worldStore);

		Hit best{};
		if (maxDistance <= 0.0f) return best;

		const auto& sbsps = worldStore.GetAllResolvedSbsps();

		CellKey cell = this->CellKeyFor(origin);

		const std::int32_t stepX = direction.X > 0.0f ? 1 : (direction.X < 0.0f ? -1 : 0);
		const std::int32_t stepY = direction.Y > 0.0f ? 1 : (direction.Y < 0.0f ? -1 : 0);
		const std::int32_t stepZ = direction.Z > 0.0f ? 1 : (direction.Z < 0.0f ? -1 : 0);

		auto boundary = [](std::int32_t cellCoord, std::int32_t step) -> float {
			return static_cast<float>(step > 0 ? cellCoord + 1 : cellCoord) * k_CellSize;
		};

		float tMaxX = stepX != 0 ? SafeDiv(boundary(cell.X, stepX) - origin.X, direction.X) : (std::numeric_limits<float>::max)();
		float tMaxY = stepY != 0 ? SafeDiv(boundary(cell.Y, stepY) - origin.Y, direction.Y) : (std::numeric_limits<float>::max)();
		float tMaxZ = stepZ != 0 ? SafeDiv(boundary(cell.Z, stepZ) - origin.Z, direction.Z) : (std::numeric_limits<float>::max)();

		const float tDeltaX = stepX != 0 ? k_CellSize / std::fabs(direction.X) : (std::numeric_limits<float>::max)();
		const float tDeltaY = stepY != 0 ? k_CellSize / std::fabs(direction.Y) : (std::numeric_limits<float>::max)();
		const float tDeltaZ = stepZ != 0 ? k_CellSize / std::fabs(direction.Z) : (std::numeric_limits<float>::max)();

		float tEnter = 0.0f;

		constexpr int k_MaxCellSteps = 4096;

		for (int steps = 0; steps < k_MaxCellSteps; ++steps)
		{
			if (best.IsHit && tEnter > best.Distance) break;

			auto it = m_Cells.find(cell);
			if (it != m_Cells.end())
			{
				for (const TriangleRef& ref : it->second)
				{
					const auto& triangle =
						sbsps[static_cast<std::size_t>(ref.SbspIndex)]
						.RenderGeometry[static_cast<std::size_t>(ref.TriangleIndex)];

					const float currentBest = best.IsHit ? best.Distance : maxDistance;

					float distance = 0.0f;
					if (!Common::Geometry::System::RayIntersectsTriangle(
						origin, direction, triangle, currentBest, distance))
					{
						continue;
					}

					if (!best.IsHit || distance < best.Distance)
					{
						best.IsHit = true;
						best.Distance = distance;
						best.Point = {
							origin.X + direction.X * distance,
							origin.Y + direction.Y * distance,
							origin.Z + direction.Z * distance
						};
					}
				}
			}

			if (tMaxX < tMaxY && tMaxX < tMaxZ)
			{
				if (tMaxX > maxDistance) break;
				tEnter = tMaxX;
				cell.X += stepX;
				tMaxX += tDeltaX;
			}
			else if (tMaxY < tMaxZ)
			{
				if (tMaxY > maxDistance) break;
				tEnter = tMaxY;
				cell.Y += stepY;
				tMaxY += tDeltaY;
			}
			else
			{
				if (tMaxZ > maxDistance) break;
				tEnter = tMaxZ;
				cell.Z += stepZ;
				tMaxZ += tDeltaZ;
			}
		}

		return best;
	}

	auto SbspRaycaster::Cleanup() -> void
	{
		m_Cells.clear();
		m_GridBuilt = false;
	}
}