export module Tables.Object.Type:Profile;

import Resolved.Definitions.Type;

namespace
{
    using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
}

export namespace Tables::Object::Type::Profile
{
    // Identification of a game engine's object.
    struct Profile
    {
        ::ObjectKind ObjectKind{ ::ObjectKind::Invalid };

        bool HasBipd{ false };
        bool HasBloc{ false };
        bool HasColl{ false };
        bool HasCtrl{ false };
        bool HasEqip{ false };
        bool HasHlmt{ false };
        bool HasMach{ false };
        bool HasMode{ false };
        bool HasProj{ false };
        bool HasScen{ false };
        bool HasVehi{ false };
        bool HasWeap{ false };
    };
}