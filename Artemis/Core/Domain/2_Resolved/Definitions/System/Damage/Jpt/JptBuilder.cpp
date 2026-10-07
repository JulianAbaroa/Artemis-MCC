module Resolved.Definitions.System;
import :Jpt;

namespace Resolved::Definitions::System
{
    auto JptBuilder::Build(const JptObject& jpt) -> Jpt
    {
        Jpt out{};

        out.TagName = jpt.TagName;

        return out;
    }
}