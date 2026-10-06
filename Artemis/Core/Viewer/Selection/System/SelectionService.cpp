module Viewer.Selection.System;

import Viewer.Selection.State;
import std;

namespace
{
    using Viewer::Selection::State::k_NoSelection;
}

namespace Viewer::Selection::System
{
    auto SelectionService::Update(const std::shared_ptr<const Tick>& tick, const Ray& ray) -> void
    {
        if (tick) this->Rebuild(tick);

        this->Pick(ray);
    }

    auto SelectionService::GetSelected() const -> std::uint32_t
    {
        return m_SelectionStore.GetSelected();
    }

    auto SelectionService::Clear() -> void
    {
        m_SelectionStore.Clear();
    }

    auto SelectionService::Reset() -> void
    {
        m_MeshBounds.clear();
        std::vector<ObjectBounds>().swap(m_Bounds);

        m_LastGeneration = 0;
        m_IsGenerationSet = false;
    }

    auto SelectionService::Rebuild(const std::shared_ptr<const Tick>& tick) -> void
    {
        if (m_IsGenerationSet && tick->Generation == m_LastGeneration) return;

        m_LastGeneration = tick->Generation;
        m_IsGenerationSet = true;

        m_Bounds.clear();

        if (!tick->Collidables) return;

        m_Bounds.reserve(tick->Collidables->size());

        auto localOf = [this](const auto* mesh) -> const LocalBounds*
        {
            const auto [it, inserted] = m_MeshBounds.try_emplace(static_cast<const void*>(mesh));
            LocalBounds& local = it->second;
            if (!inserted) return local.Valid ? &local : nullptr;

            const auto& triangles = mesh->Triangles;
            if (triangles.empty()) return nullptr;

            local.Valid = true;
            local.Min = { triangles[0].A.X, triangles[0].A.Y, triangles[0].A.Z };
            local.Max = local.Min;

            auto grow = [&local](const auto& point)
            {
                local.Min[0] = (std::min)(local.Min[0], point.X);
                local.Min[1] = (std::min)(local.Min[1], point.Y);
                local.Min[2] = (std::min)(local.Min[2], point.Z);
                local.Max[0] = (std::max)(local.Max[0], point.X);
                local.Max[1] = (std::max)(local.Max[1], point.Y);
                local.Max[2] = (std::max)(local.Max[2], point.Z);
            };

            for (const auto& triangle : triangles)
            {
                grow(triangle.A);
                grow(triangle.B);
                grow(triangle.C);
            }

            return &local;
        };

        for (const auto& collidable : *tick->Collidables)
        {
            ObjectBounds bounds{};
            bounds.Handle = collidable.Handle;
            bool hasBounds = false;

            for (const auto& part : collidable.Parts)
            {
                if (!part.Source) continue;

                const LocalBounds* local = localOf(part.Source);
                if (!local) continue;

                const auto& m = part.Transform;

                for (int corner = 0; corner < 8; ++corner)
                {
                    const float x = (corner & 1) ? local->Max[0] : local->Min[0];
                    const float y = (corner & 2) ? local->Max[1] : local->Min[1];
                    const float z = (corner & 4) ? local->Max[2] : local->Min[2];

                    const float wx = m[0] * x + m[1] * y + m[2] * z + m[3];
                    const float wy = m[4] * x + m[5] * y + m[6] * z + m[7];
                    const float wz = m[8] * x + m[9] * y + m[10] * z + m[11];

                    if (!hasBounds)
                    {
                        bounds.Min.X = bounds.Max.X = wx;
                        bounds.Min.Y = bounds.Max.Y = wy;
                        bounds.Min.Z = bounds.Max.Z = wz;
                        hasBounds = true;
                        continue;
                    }

                    bounds.Min.X = (std::min)(bounds.Min.X, wx);
                    bounds.Min.Y = (std::min)(bounds.Min.Y, wy);
                    bounds.Min.Z = (std::min)(bounds.Min.Z, wz);
                    bounds.Max.X = (std::max)(bounds.Max.X, wx);
                    bounds.Max.Y = (std::max)(bounds.Max.Y, wy);
                    bounds.Max.Z = (std::max)(bounds.Max.Z, wz);
                }
            }

            if (hasBounds) m_Bounds.push_back(bounds);
        }
    }

    auto SelectionService::Pick(const Ray& ray) -> void
    {
        if (!m_SelectionStore.ConsumePick()) return;

        std::uint32_t best = k_NoSelection;
        float bestDistance = (std::numeric_limits<float>::max)();

        for (const ObjectBounds& box : m_Bounds)
        {
            const float distance = this->Intersect(ray, box);

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