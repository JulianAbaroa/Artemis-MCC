export module Resolved.Definitions.System:Hlmt;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    class HlmtBuilder
    {
    private:
        using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;
        using ResolvedHlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using DamageSection = Resolved::Definitions::Type::Hlmt::DamageSection;
        using InstantResponse = Resolved::Definitions::Type::Hlmt::InstantResponse;

    public:
        explicit HlmtBuilder() = default;
        ~HlmtBuilder() = default;

        auto Build(const HlmtObject& hlmt) -> ResolvedHlmt;
    };
}