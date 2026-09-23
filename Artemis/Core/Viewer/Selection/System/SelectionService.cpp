module Viewer.Selection.System;

import std;

namespace Viewer::Selection::System
{
	auto SelectionService::Pick(const Ray& ray, std::span<const ObjectBounds> bounds) -> void
	{
		using Viewer::Selection::State::k_NoSelection;

		if (!m_SelectionStore.ConsumePick()) return;

		std::uint32_t best = k_NoSelection;
		float bestDistance = (std::numeric_limits<float>::max)();

		for (const ObjectBounds& box : bounds)
		{
			const float distance = Intersect(ray, box);

			if (distance >= 0.0f && distance < bestDistance)
			{
				bestDistance = distance;
				best = box.Handle;
			}
		}

		m_SelectionStore.SetSelected(best);
	}

	auto SelectionService::Intersect(const Ray& ray, const ObjectBounds& bounds) -> float
	{
		const float origin[3] = { ray.Origin.X, ray.Origin.Y, ray.Origin.Z };
		const float direction[3] = { ray.Direction.X, ray.Direction.Y, ray.Direction.Z };
		const float low[3] = { bounds.Min.X, bounds.Min.Y, bounds.Min.Z };
		const float high[3] = { bounds.Max.X, bounds.Max.Y, bounds.Max.Z };

		float tMin = 0.0f;
		float tMax = 1e30f;

		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(direction[axis]) < 1e-8f)
			{
				if (origin[axis] < low[axis] || origin[axis] > high[axis]) return -1.0f;
				continue;
			}

			const float inverse = 1.0f / direction[axis];
			float t1 = (low[axis] - origin[axis]) * inverse;
			float t2 = (high[axis] - origin[axis]) * inverse;
			if (t1 > t2) std::swap(t1, t2);

			tMin = (std::max)(tMin, t1);
			tMax = (std::min)(tMax, t2);

			if (tMin > tMax) return -1.0f;
		}

		return tMin;
	}
}