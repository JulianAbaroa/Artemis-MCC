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

        bool HasColl{ false };
        bool HasHlmt{ false };
        bool HasMode{ false };
    };
}