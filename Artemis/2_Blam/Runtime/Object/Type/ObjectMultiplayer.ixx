export module Blam.Runtime.Object.Type:Multiplayer;

import Common.Team.Type;
import Common.ZoneShape.Type;

namespace
{
    using Common::Team::Type::Team;
    using Common::ZoneShape::Type::ZoneShape;
}

export namespace Blam::Runtime::Object::Type::Multiplayer
{
    // Basic data of a multiplayer object: a zone shape and a team.
    // note: Any object can carry them, but most keep the shape in None. They are read from the multiplayer block of the datum.
    struct Multiplayer
    {
        ZoneShape Shape{};
        Team Team{};
    };
}