export module Resolved.Definitions.Type:Sddt;

import :Scnr;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Triangle;
    using Resolved::Definitions::Type::Scnr::SoftCeilingKind;
}

export namespace Resolved::Definitions::Type::Sddt
{
    // Surface that the scenario places as a soft ceiling, with the triangles that make it.
    struct SoftCeilingMesh
    {
        // Name as a string id. There is no string table reader yet.
        std::uint32_t Name{};

        SoftCeilingKind Kind{ SoftCeilingKind::Invalid };

        std::vector<Triangle> Triangles{};
    };

    // Holds the soft ceilings of a structure design.
    struct Sddt
    {
        std::string TagName{};

        std::vector<SoftCeilingMesh> SoftCeilings{};
    };
}