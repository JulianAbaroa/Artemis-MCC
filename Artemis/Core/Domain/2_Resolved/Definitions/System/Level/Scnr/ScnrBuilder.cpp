module Resolved.Definitions.System;
import :Scnr;

import Common.Math.Type;
import std;

namespace Resolved::Definitions::System
{
    auto ScnrBuilder::Build(const ScnrObject& scnr) -> ResolvedScnr
    {
        ResolvedScnr out{};

        out.TagName = scnr.TagName;

        return out;
    }
}