export module Resolved.Definitions.Type:Sbsp;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Triangle;
}

export namespace Resolved::Definitions::Type::Sbsp
{
    // Holds the bounds and the render triangles of a structure bsp.
    struct Sbsp
    {
        std::string TagName{};

        Vec3 WorldBoundsMin{};
        Vec3 WorldBoundsMax{};
        Vec3 MoppBoundsMin{};
        Vec3 MoppBoundsMax{};

        std::vector<Triangle> RenderGeometry{};
    };
}