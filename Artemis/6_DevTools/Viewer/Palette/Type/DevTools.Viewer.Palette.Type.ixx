export module DevTools.Viewer.Palette.Type;

import DevTools.Viewer.Style.Type;
import std;

export namespace DevTools::Viewer::Palette::Type
{
    // Color assigned to an object handle.
    // Rank is the order in which the entry was added, and a higher rank wins over a lower one.
    struct Entry
    {
        std::uint32_t Handle{};
        std::uint32_t Rank{};
        DevTools::Viewer::Style::Type::Color Value{};
    };
}