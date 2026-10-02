module Resolved.Definitions.System;
import :Jpt;

import std;

namespace Resolved::Definitions::System
{
    auto JptBuilder::Build(const JptObject& jpt) -> ResolvedJpt
    {
        ResolvedJpt out{};
        const auto& data = jpt.Data;

        out.TagName = jpt.TagName;

        return out;
    }
}