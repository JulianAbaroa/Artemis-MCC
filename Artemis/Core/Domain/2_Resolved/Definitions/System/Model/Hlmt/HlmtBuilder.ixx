export module Resolved.Definitions.System:Hlmt;

import Map.Reader.System;
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
        using TagResolverService = Map::Reader::System::TagResolverService;

    public:
        explicit HlmtBuilder(TagResolverService& tagResolverService) :
            m_TagResolverService(tagResolverService) {}
        ~HlmtBuilder() = default;

        auto Build(const HlmtObject& hlmt) -> ResolvedHlmt;

    private:
        TagResolverService& m_TagResolverService;
    };
}