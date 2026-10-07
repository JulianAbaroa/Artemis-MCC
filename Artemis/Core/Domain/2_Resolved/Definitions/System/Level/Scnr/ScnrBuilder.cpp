module Resolved.Definitions.System;
import :Scnr;

namespace Resolved::Definitions::System
{
    auto ScnrBuilder::Build(const ScnrObject& scnr) -> Scnr
    {
        Scnr out{};

        out.TagName = scnr.TagName;

        return out;
    }
}