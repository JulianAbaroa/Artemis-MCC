export module Resolved.Definitions.System:Sddt;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the structure design definition.
    class SddtBuilder
    {
    private:
        using SddtObject = Map::Tag::Type::Sddt::Object::SddtObject;
        using Sddt = Resolved::Definitions::Type::Sddt::Sddt;

    public:
        SddtBuilder() = default;
        ~SddtBuilder() = default;

        auto Build(const SddtObject& sddt) -> Sddt;
    };
}