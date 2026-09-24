export module Tables.Object.Type:Profile;

import :Class;

namespace
{
    using Class = Tables::Object::Type::Class::Class;
}

export namespace Tables::Object::Type::Profile
{
    // Identification of a game engine's object.
    struct Profile
    {
        ::Class Class{ ::Class::Invalid };

        bool HasBipd{ false };
        bool HasBloc{ false };
        bool HasColl{ false };
        bool HasCtrl{ false };
        bool HasEqip{ false };
        bool HasHlmt{ false };
        bool HasMach{ false };
        bool HasMode{ false };
        bool HasPhmo{ false };
        bool HasProj{ false };
        bool HasScen{ false };
        bool HasScnr{ false };
        bool HasVehi{ false };
        bool HasWeap{ false };
    };
}