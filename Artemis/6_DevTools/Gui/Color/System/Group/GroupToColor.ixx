module;

#include "External/imgui/imgui.h"

export module DevTools.Gui.Color.System:Group;

import Common.Semantic.Type;

export namespace DevTools::Gui::Color::System
{
    // Picks the display color of a group (D53: one pastel per group).
    class GroupToColor
    {
    private:
        using Group = Common::Semantic::Type::Group;

    public:
        GroupToColor() = default;
        ~GroupToColor() = default;

        // return: The pastel of the group. Grey for Unknown.
        static auto FromGroup(Group group) -> ImVec4;
    };
}