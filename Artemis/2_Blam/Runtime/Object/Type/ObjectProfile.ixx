export module Blam.Runtime.Object.Type:Profile;

import Blam.MapModel.Definitions.Type;

namespace
{
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
}

export namespace Blam::Runtime::Object::Type::Profile
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