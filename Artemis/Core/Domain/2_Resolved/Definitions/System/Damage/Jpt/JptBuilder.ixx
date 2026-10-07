export module Resolved.Definitions.System:Jpt;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the damage effect definition.
    // Waiting for some system to need this.
    class JptBuilder
    {
    private:
        using JptObject = Map::Tag::Type::Jpt::Object::JptObject;
        using Jpt = Resolved::Definitions::Type::Jpt::Jpt;

    public:
        JptBuilder() = default;
        ~JptBuilder() = default;

        auto Build(const JptObject& jpt) -> Jpt;
    };
}