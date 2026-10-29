export module Blam.MapModel.Definitions.System:Hlmt;

import Blam.Map.Reader.System;
import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;
import std;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the model definition.
    class HlmtBuilder
    {
    private:
        using HlmtObject = Blam::Map::Tag::Type::Hlmt::Object::HlmtObject;
        using Hlmt = Blam::MapModel::Definitions::Type::Hlmt::Hlmt;
        using DamageSection = Blam::MapModel::Definitions::Type::Hlmt::DamageSection;
        using InstantResponse = Blam::MapModel::Definitions::Type::Hlmt::InstantResponse;
        using TagReferenceService = Blam::Map::Reader::System::TagReferenceService;

    public:
        explicit HlmtBuilder(TagReferenceService& tagReferenceService) :
            m_TagReferenceService(tagReferenceService) {}
        ~HlmtBuilder() = default;

        // note: The model tag names are resolved here, so a missing reference leaves the name empty.
        auto Build(const HlmtObject& hlmt) -> Hlmt;

    private:
        TagReferenceService& m_TagReferenceService;
    };
}