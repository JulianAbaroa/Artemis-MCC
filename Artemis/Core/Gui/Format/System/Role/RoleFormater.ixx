export module Gui.Format.System:Role;

import Relations.Classifier.Type;

export namespace Gui::Format::System
{
    // Converts the role enum to display text. Unlisted values give "Unknown".
    class RoleFormater
    {
    private:
        using Role = Relations::Classifier::Type::Role;

    public:
        RoleFormater() = default;
        ~RoleFormater() = default;

        static auto RoleToString(Role role) -> const char*;
    };
}