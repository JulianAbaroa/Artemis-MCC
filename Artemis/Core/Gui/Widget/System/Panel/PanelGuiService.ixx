module;

#include "External/imgui/imgui.h"

export module Gui.Widget.System:Panel;

import Common.Math.Type;
import std;

export namespace Gui::Widget::System
{
    // Draws the small building blocks shared by the HUD panels.
    class PanelGuiService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;

    public:
        PanelGuiService() = default;
        ~PanelGuiService() = default;

        static auto DrawVec3(const char* label, const Vec3& value) -> void;

        // Draws the label with "yes" in the given color, or "no" in grey.
        static auto DrawBoolBadge(const char* label, bool value, const ImVec4& trueColor) -> void;

        // Draws a separator followed by the title, if any.
        static auto DrawSectionSeparator(const char* title = nullptr) -> void;

        // Draws the colored title, then the tag name and the handle in hex.
        static auto DrawHeader(const ImVec4& color, const char* title,
            const std::string& tagName, std::uint32_t handle) -> void;
    };
}