#pragma once

#include "../GpuPipeline.h"
#include "Core/Types/ZoneShape.h"

#include "CollidableClassifier.h"

#include <vector>
#include <array>
#include <cmath>

namespace ZoneGeometry
{
    inline void BuildBasis(const std::array<float, 3>& forward,
        const std::array<float, 3>& up,
        std::array<float, 3>& outRight,
        std::array<float, 3>& outFwd,
        std::array<float, 3>& outUp)
    {
        auto norm = [](std::array<float, 3> v) {
            const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            if (l > 1e-6f) { v[0] /= l; v[1] /= l; v[2] /= l; }
            return v;
            };
        auto cross = [](const std::array<float, 3>& a, const std::array<float, 3>& b) {
            return std::array<float, 3>{
                a[1] * b[2] - a[2] * b[1],
                    a[2] * b[0] - a[0] * b[2],
                    a[0] * b[1] - a[1] * b[0] };
            };

        outFwd = norm(forward);
        outUp = norm(up);
        outRight = norm(cross(outFwd, outUp));
        outUp = norm(cross(outRight, outFwd));
    }

    inline std::array<float, 3> ToWorld(
        const std::array<float, 3>& center,
        const std::array<float, 3>& right,
        const std::array<float, 3>& fwd,
        const std::array<float, 3>& up,
        float lr, float lf, float lu)
    {
        return {
            center[0] + right[0] * lr + fwd[0] * lf + up[0] * lu,
            center[1] + right[1] * lr + fwd[1] * lf + up[1] * lu,
            center[2] + right[2] * lr + fwd[2] * lf + up[2] * lu };
    }

    void AppendSolid(std::vector<GpuPipeline::Vertex>& out,
        const std::array<float, 3>& position,
        const std::array<float, 3>& forward,
        const std::array<float, 3>& up,
        const ZoneShape& zone,
        const RgbColor& color);

    void AppendWire(std::vector<GpuPipeline::Vertex>& out,
        const std::array<float, 3>& position,
        const std::array<float, 3>& forward,
        const std::array<float, 3>& up,
        const ZoneShape& zone,
        const RgbColor& color);
}