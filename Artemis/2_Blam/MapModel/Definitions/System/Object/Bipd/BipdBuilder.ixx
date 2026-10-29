export module Blam.MapModel.Definitions.System:Bipd;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the biped definition.
    class BipdBuilder
    {
    private:
        using BipdObject = Blam::Map::Tag::Type::Bipd::Object::BipdObject;
        using Bipd = Blam::MapModel::Definitions::Type::Bipd::Bipd;

    public:
        explicit BipdBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~BipdBuilder() = default;

        auto Build(const BipdObject& bipd) -> Bipd;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}