#include "pch.h"

#include "ZoneGeometry.h"

namespace
{
    constexpr int kCylinderSegments = 24;

    using V = GpuPipeline::Vertex;

    V MakeVert(const std::array<float, 3>& p, const RgbColor& c)
    {
        return { p[0], p[1], p[2], c.r, c.g, c.b };
    }
}

namespace ZoneGeometry
{
    void AppendSolid(std::vector<V>& out,
        const std::array<float, 3>& position,
        const std::array<float, 3>& forward,
        const std::array<float, 3>& up,
        const ZoneShape& zone,
        const RgbColor& color)
    {
        std::array<float, 3> r, f, u;
        BuildBasis(forward, up, r, f, u);

        const float top = zone.Top;
        const float bot = -zone.Bottom;

        auto W = [&](float lr, float lf, float lu) {
            return ToWorld(position, r, f, u, lr, lf, lu);
            };

        if (zone.ShapeType == ShapeType::Cylinder)
        {
            const float rad = zone.Radius;
            const int N = kCylinderSegments;

            for (int i = 0; i < N; ++i)
            {
                const float a0 = (float(i) / N) * 6.2831853f;
                const float a1 = (float(i + 1) / N) * 6.2831853f;

                const float x0 = std::cos(a0) * rad, y0 = std::sin(a0) * rad;
                const float x1 = std::cos(a1) * rad, y1 = std::sin(a1) * rad;

                const auto bl = W(x0, y0, bot);
                const auto br = W(x1, y1, bot);
                const auto tl = W(x0, y0, top);
                const auto tr = W(x1, y1, top);

                out.push_back(MakeVert(bl, color));
                out.push_back(MakeVert(br, color));
                out.push_back(MakeVert(tr, color));
                out.push_back(MakeVert(bl, color));
                out.push_back(MakeVert(tr, color));
                out.push_back(MakeVert(tl, color));

                const auto cTop = W(0, 0, top);
                out.push_back(MakeVert(cTop, color));
                out.push_back(MakeVert(tl, color));
                out.push_back(MakeVert(tr, color));

                const auto cBot = W(0, 0, bot);
                out.push_back(MakeVert(cBot, color));
                out.push_back(MakeVert(br, color));
                out.push_back(MakeVert(bl, color));
            }
        }
        else if (zone.ShapeType == ShapeType::Box)
        {
            const float hw = zone.Radius * 0.5f;
            const float hl = zone.Length * 0.5f;

            const std::array<std::array<float, 3>, 8> c = {
                W(-hw,-hl,bot), W(hw,-hl,bot), W(hw, hl,bot), W(-hw, hl,bot),
                W(-hw,-hl,top), W(hw,-hl,top), W(hw, hl,top), W(-hw, hl,top)
            };

            auto quad = [&](int a, int b, int d, int e) {
                out.push_back(MakeVert(c[a], color));
                out.push_back(MakeVert(c[b], color));
                out.push_back(MakeVert(c[d], color));
                out.push_back(MakeVert(c[a], color));
                out.push_back(MakeVert(c[d], color));
                out.push_back(MakeVert(c[e], color));
                };

            quad(0, 1, 2, 3);
            quad(4, 5, 6, 7);
            quad(0, 1, 5, 4);
            quad(2, 3, 7, 6);
            quad(1, 2, 6, 5);
            quad(3, 0, 4, 7);
        }
    }

    void AppendWire(std::vector<V>& out,
        const std::array<float, 3>& position,
        const std::array<float, 3>& forward,
        const std::array<float, 3>& up,
        const ZoneShape& zone,
        const RgbColor& color)
    {
        std::array<float, 3> r, f, u;
        BuildBasis(forward, up, r, f, u);

        const float top = zone.Top;
        const float bot = -zone.Bottom;

        auto W = [&](float lr, float lf, float lu) {
            return ToWorld(position, r, f, u, lr, lf, lu);
            };
        auto line = [&](const std::array<float, 3>& a, const std::array<float, 3>& b) {
            out.push_back(MakeVert(a, color));
            out.push_back(MakeVert(b, color));
            };

        if (zone.ShapeType == ShapeType::Cylinder)
        {
            const float rad = zone.Radius;
            const int N = kCylinderSegments;

            std::array<float, 3> prevTop{}, prevBot{};
            for (int i = 0; i <= N; ++i)
            {
                const float a = (float(i % N) / N) * 6.2831853f;
                const float x = std::cos(a) * rad, y = std::sin(a) * rad;
                const auto pTop = W(x, y, top);
                const auto pBot = W(x, y, bot);

                if (i > 0)
                {
                    line(prevTop, pTop);
                    line(prevBot, pBot);
                }
                prevTop = pTop; prevBot = pBot;

                if (i % 6 == 0) line(pTop, pBot);
            }
        }
        else if (zone.ShapeType == ShapeType::Box)
        {
            const float hw = zone.Radius * 0.5f;
            const float hl = zone.Length * 0.5f;

            const std::array<std::array<float, 3>, 8> c = {
                W(-hw,-hl,bot), W(hw,-hl,bot), W(hw, hl,bot), W(-hw, hl,bot),
                W(-hw,-hl,top), W(hw,-hl,top), W(hw, hl,top), W(-hw, hl,top)
            };

            int edges[12][2] = {
                {0,1},{1,2},{2,3},{3,0},
                {4,5},{5,6},{6,7},{7,4},
                {0,4},{1,5},{2,6},{3,7}
            };
            for (auto& e : edges) line(c[e[0]], c[e[1]]);
        }
    }
}