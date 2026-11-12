module;

#include "External/imgui/imgui.h"

export module DevTools.Gui.Widget.System:DisabledScope;

import std;

export namespace DevTools::Gui::Widget::System
{
    // Disables the widgets created while it lives and remembers why, so their tooltip can say it.
    class DisabledScope
    {
    public:
        // param isDisabled: Whether the widgets cannot act now.
        // param reason: Tooltip of the widgets while they are disabled. Must outlive the scope (a literal).
        DisabledScope(bool isDisabled, const char* reason);
        ~DisabledScope();

        DisabledScope(const DisabledScope&) = delete;
        auto operator=(const DisabledScope&) -> DisabledScope& = delete;

        // Shows the reason while the last item is disabled and hovered, or enabledText while it is enabled and hovered.
        auto Tooltip(const char* enabledText) const -> void;

    private:
        bool m_IsDisabled{ false };
        const char* m_Reason{ nullptr };
    };
}