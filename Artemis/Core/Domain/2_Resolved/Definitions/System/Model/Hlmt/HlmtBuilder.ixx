export module Resolved.Definitions.System:Hlmt;

import Map.Reader.System;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
    // Builds the model definition.
    class HlmtBuilder
    {
    private:
        using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;
        using Hlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using DamageSection = Resolved::Definitions::Type::Hlmt::DamageSection;
        using InstantResponse = Resolved::Definitions::Type::Hlmt::InstantResponse;
        using TagResolverService = Map::Reader::System::TagResolverService;

    public:
        explicit HlmtBuilder(TagResolverService& tagResolverService) :
            m_TagResolverService(tagResolverService) {}
        ~HlmtBuilder() = default;

        // note: The model tag names are resolved here, so a missing reference leaves the name empty.
        auto Build(const HlmtObject& hlmt) -> Hlmt;

    private:
        TagResolverService& m_TagResolverService;
    };
}