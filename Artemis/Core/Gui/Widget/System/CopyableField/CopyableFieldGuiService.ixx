export module Gui.Widget.System:CopyableField;

import Gui.Style.Type;
import std;

namespace
{
    using Gui::Style::Type::k_AnimationDuration;
}

export namespace Gui::Widget::System
{
    // Draws a read-only text field with a label. Right-click copies the value and flashes the field.
    // note: Keeps the state of one flash, so a second copy replaces the flash of the previous one.
    class CopyableFieldGuiService
    {
    public:
        CopyableFieldGuiService() = default;
        ~CopyableFieldGuiService() = default;

        // param ownerHandle: Makes the ImGui id unique when several owners draw the same label.
        auto Draw(const char* label, const std::string& value,
            std::uint32_t ownerHandle) -> void;

    private:
        std::string m_AnimateCopyLabel{};
        float m_AnimationStartTime{ 0.0f };
    };
}