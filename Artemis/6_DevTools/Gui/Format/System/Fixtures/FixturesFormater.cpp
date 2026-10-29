module DevTools.Gui.Format.System;
import :Fixtures;

namespace DevTools::Gui::Format::System
{
    auto FixturesFormater::ZoneKindToString(ZoneShapeKind kind) -> const char*
    {
        switch (kind)
        {
        case ZoneShapeKind::None:     return "None";
        case ZoneShapeKind::Cylinder: return "Cylinder";
        case ZoneShapeKind::Box:      return "Box";
        default:                 return "Unknown";
        }
    }
}