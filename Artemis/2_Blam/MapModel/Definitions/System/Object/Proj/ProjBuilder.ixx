export module Blam.MapModel.Definitions.System:Proj;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the projectile definition.
    class ProjBuilder
    {
    private:
        using ProjObject = Blam::Map::Tag::Type::Proj::Object::ProjObject;
        using Proj = Blam::MapModel::Definitions::Type::Proj::Proj;

    public:
        explicit ProjBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~ProjBuilder() = default;

        auto Build(const ProjObject& proj) -> Proj;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}