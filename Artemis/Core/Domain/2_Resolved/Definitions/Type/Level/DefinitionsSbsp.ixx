export module Resolved.Definitions.Type:Sbsp;

import Common.Math.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Triangle = Common::Math::Type::Triangle;
}

export namespace Resolved::Definitions::Type::Sbsp
{
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