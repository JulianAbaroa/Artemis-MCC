export module Resolved.Definitions.Reflect:Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Object::MultiplayerObject;
    using Resolved::Definitions::Type::Object::Object;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<MultiplayerObject>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
            MakeField("ObjectKind", &MultiplayerObject::MultiplayerObjectKind),
        };
    };

    template <>
    struct Fields<Object>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
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