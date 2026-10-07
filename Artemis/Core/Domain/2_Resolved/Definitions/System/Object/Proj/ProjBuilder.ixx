export module Resolved.Definitions.System:Proj;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the projectile definition.
    class ProjBuilder
    {
    private:
        using ProjObject = Map::Tag::Type::Proj::Object::ProjObject;
        using Proj = Resolved::Definitions::Type::Proj::Proj;

    public:
        explicit ProjBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~ProjBuilder() = default;

        auto Build(const ProjObject& proj) -> Proj;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}