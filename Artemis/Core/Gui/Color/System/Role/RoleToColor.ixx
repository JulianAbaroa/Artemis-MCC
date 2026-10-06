module;

#include "External/imgui/imgui.h"

export module Gui.Color.System:Role;

import Relations.Classifier.Type;

export namespace Gui::Color::System
{
    // Picks the display color of a role.
    class RoleToColor
    {
    private:
        using Role = Relations::Classifier::Type::Role;

    public:
        RoleToColor() = default;
        ~RoleToColor() = default;

        // return: The color of the role group. Grey for the roles without a group.
        static auto FromRole(Role role) -> ImVec4;
    };
}