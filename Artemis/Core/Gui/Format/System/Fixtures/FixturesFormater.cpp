module Gui.Format.System;
import :Fixtures;

namespace Gui::Format::System
{
    auto FixturesFormater::ZoneKindToString(ZoneKind kind) -> const char*
    {
        switch (kind)
        {
        case ZoneKind::None:     return "None";
        case ZoneKind::Cylinder: return "Cylinder";
        case ZoneKind::Box:      return "Box";
        default:                 return "Unknown";
        }
    }

    auto FixturesFormater::DestructibleKindToString(DestructibleKind kind) -> const char*
    {
        switch (kind)
        {
        case DestructibleKind::Explosive:      return "Explosive";
        case DestructibleKind::Pallet:         return "Pallet";
        case DestructibleKind::PortableShield: return "Portable Shield";
        default:                               return "Unknown";
        }
    }
}