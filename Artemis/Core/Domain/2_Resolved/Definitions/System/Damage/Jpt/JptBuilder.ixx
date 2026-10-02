export module Resolved.Definitions.System:Jpt;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
	class JptBuilder
	{
    private:
        using JptObject = Map::Tag::Type::Jpt::Object::JptObject;
        using ResolvedJpt = Resolved::Definitions::Type::Jpt::Jpt;

    public:
        JptBuilder() = default;
        ~JptBuilder() = default;

        auto Build(const JptObject& jpt) -> ResolvedJpt;
	};
}