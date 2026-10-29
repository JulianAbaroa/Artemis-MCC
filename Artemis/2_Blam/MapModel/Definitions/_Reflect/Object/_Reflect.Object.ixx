export module Blam.MapModel.Definitions.Reflect:Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Object::MultiplayerObject;
    using Blam::MapModel::Definitions::Type::Object::Object;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<MultiplayerObject>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("ObjectKind", &MultiplayerObject::MultiplayerObjectKind),
        };
    };

    template <>
    struct Fields<Object>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("TagName", &Object::TagName),
            MakeField("ObjectKind", &Object::ObjectKind),
            MakeField("BoundingRadius", &Object::BoundingRadius),
            MakeField("BoundingOffset", &Object::BoundingOffset),
            MakeField("CollisionDamageTagName", &Object::CollisionDamageTagName),
            MakeField("BrittleCollisionDamageTagName", &Object::BrittleCollisionDamageTagName),
            MakeField("ModelTagName", &Object::ModelTagName),
            MakeField("MultiplayerObjects", &Object::MultiplayerObjects),
        };
    };
}